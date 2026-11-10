#!/usr/bin/env python3
"""Offline legacy clan import. Dry-run by default; no runtime authority switch."""
from pathlib import Path
import argparse,hashlib,io,json,os,re,secrets,struct,subprocess,sys,tempfile,time
from build_clan_repository_tool import ROOT,build
MAX64=(1<<63)-1
FIELDS=re.findall(r'X\((\w+),(\w+),(\w+)\)',(ROOT/'src/clan_legacy_fields.h').read_text())
class ImportProblem(ValueError):pass
def integer(value,label,minimum=0,maximum=MAX64):
 if not re.fullmatch(r'-?\d+',value.strip()):raise ImportProblem(f'{label}: expected an integer')
 n=int(value)
 if not minimum<=n<=maximum:raise ImportProblem(f'{label}: integer out of range')
 return n
def normalized_clan(n):return None if n in (0,65535) else n
def flags(value):
 if re.fullmatch(r'\d+',value):return int(value)
 if not re.fullmatch(r'[a-zA-Z]+',value):raise ImportProblem('Invalid player index flags')
 return sum(1<<(ord(c)-ord('a') if c.islower() else ord(c)-ord('A')+26) for c in set(value))
class Source:
 def __init__(self):self.files={}
 def read(self,path,key):
  path=Path(path)
  if path.is_symlink() or not path.is_file():raise ImportProblem(f'Missing or symlinked input: {path}')
  data=path.read_bytes()
  if b'\0' in data:raise ImportProblem(f'NUL in input: {path}')
  try:text=data.decode('utf-8')
  except UnicodeDecodeError:raise ImportProblem(f'Input must have reviewed UTF-8 encoding: {path}')
  self.files[key]=(path,data)
  return text
 def digests(self):return {k:hashlib.sha256(data).hexdigest() for k,(p,data) in sorted(self.files.items())}
 def digest(self):return hashlib.sha256(json.dumps(self.digests(),sort_keys=True,separators=(',',':')).encode()).hexdigest()
 def verify(self):
  for key,(path,data) in self.files.items():
   if path.is_symlink() or path.read_bytes()!=data:raise ImportProblem(f'Source changed during import: {key}')
 def backup(self,destination,digest,mapping):
  destination=Path(destination).resolve()
  for p,data in self.files.values():
   if destination==p.resolve() or p.resolve() in destination.parents:raise ImportProblem('Backup destination overlaps source')
  destination.mkdir(mode=0o700,parents=True,exist_ok=True)
  target=destination/(digest+'-'+secrets.token_hex(4));target.mkdir(mode=0o700)
  for key,(path,data) in self.files.items():
   out=target/key;out.parent.mkdir(mode=0o700,parents=True,exist_ok=True)
   fd=os.open(out,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
   with os.fdopen(fd,'wb') as f:f.write(data);f.flush();os.fsync(f.fileno())
  fd=os.open(target/'manifest.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
  with os.fdopen(fd,'wb') as f:f.write((json.dumps(dict(digest=digest,files=self.digests(),source_paths={k:str(p.resolve()) for k,(p,d) in self.files.items()},mapping=mapping),indent=2)+'\n').encode());f.flush();os.fsync(f.fileno())
  for path in sorted([x for x in target.rglob('*') if x.is_dir()]+[target,destination],key=lambda x:len(x.parts),reverse=True):
   fd=os.open(path,os.O_RDONLY);os.fsync(fd);os.close(fd)
  fd=os.open(target,os.O_RDONLY);os.fsync(fd);os.close(fd)
  return str(target)
def legacy_description(raw):
 # Match production fread_string(): fgets(...,512), trailing-tilde termination,
 # CRLF continuations, and parse_at(). Keep original bytes in the backup.
 stream=io.BytesIO(raw.encode('utf-8'));parts=[]
 while True:
  piece=stream.readline(511)
  if not piece:raise ImportProblem('Unterminated legacy description')
  value=piece.rstrip(b'\r\n')
  if value.endswith(b'~'):
   parts.append(value[:-1])
   if stream.read().strip(b'\r\n'):raise ImportProblem('Native description terminates inside a long line; reconcile the source block')
   break
  parts.append(value+b'\r\n')
 data=b''.join(parts)
 if len(data)>=49152:raise ImportProblem('Legacy description exceeds native reader capacity')
 try:value=data.decode('utf-8')
 except UnicodeDecodeError:raise ImportProblem('Legacy reader splits a UTF-8 character; review source wrapping/encoding')
 out=[];i=0
 while i<len(value):
  if value[i]=='@' and value[i:i+2]=='@@':out.append('@@');i+=2
  elif value[i]=='@':out.append('\t');i+=1
  else:out.append(value[i]);i+=1
 return ''.join(out)
def records(text,kind):
 lines=[x.rstrip('\r') for x in text.split('\n')];out=[];current=None;i=0;terminated=False
 while i<len(lines):
  line=lines[i];i+=1
  if not line or line.startswith('*'):continue
  if line=='$':terminated=True;break
  if line.startswith('#'):
   if current:out.append(current)
   current={'vnum':integer(line[1:],kind+' vnum',0,65534)};continue
  if current is None:raise ImportProblem(f'{kind}: data before first record')
  match=re.fullmatch(r'([A-Za-z ]{4}):(.*)',line)
  if not match:raise ImportProblem(f'{kind}: malformed line {line[:60]}')
  tag,value=match.groups();tag=tag.strip();value=value.lstrip(': ')
  if tag in current:raise ImportProblem(f'{kind}: duplicate tag {tag}')
  if tag in ('Desc','Rank','Priv','Popu'):
   if value:raise ImportProblem(f'{kind}: unexpected inline block {tag}')
   block=[]
   while i<len(lines):
    value=lines[i];i+=1
    if tag=='Desc':
     block.append(value+'\n')
     if value.endswith('~'):break
    else:
     if value.startswith('~'):
      if value!='~':raise ImportProblem(f'{kind}: unexpected text after block terminator')
      break
     if value and not value.startswith('*'):block.append(value)
   else:raise ImportProblem(f'{kind}: unterminated {tag}')
   current[tag]=legacy_description(''.join(block)) if tag=='Desc' else block
  else:current[tag]=value
 if not terminated or any(x.strip() for x in lines[i:]):raise ImportProblem(f'{kind}: missing terminator or trailing records')
 if current:out.append(current)
 if len({x['vnum'] for x in out})!=len(out):raise ImportProblem(f'{kind}: duplicate vnum')
 return out
def player_tags(text):
 lines=[x.rstrip('\r') for x in text.split('\n')];out={};skip={'Bond','BGrd','Desc','Flaw','Goal','Idel','Pers'};i=0
 while i<len(lines):
  match=re.match(r'^(.{4}):\s*(.*)$',lines[i]);i+=1
  if not match:continue
  tag,value=match.groups()
  if tag in skip:
   if value:raise ImportProblem(f'Unexpected inline player block {tag}')
   while i<len(lines) and not lines[i].endswith('~'):i+=1
   if i==len(lines):raise ImportProblem(f'Unterminated player {tag}')
   i+=1
   continue
  if tag in {'Id  ','Name','Cln ','Clrk','CPts','Act '}:
   if tag in out:raise ImportProblem(f'Duplicate player identity/membership tag {tag}')
   out[tag]=value
 return out
def player_path(root,name):
 first=name[0].lower();group='A-E' if first<='e' else 'F-J' if first<='j' else 'K-O' if first<='o' else 'P-T' if first<='t' else 'U-Z'
 return Path(root)/group/(name.lower()+'.plr')
def load_source(args):
 source=Source();clans=records(source.read(args.clans,'clans'),'clans');claims=records(source.read(args.claims,'claims'),'claims')
 if len(clans)>25:raise ImportProblem('More than 25 clans')
 investment_path=args.investments or (Path(args.clans).parent/'clan_investments')
 if args.investments or investment_path.exists():
  investments=source.read(investment_path,'clan_investments')
  if any(x.strip() and x.strip()!='$' and not x.startswith('*') for x in investments.splitlines()):raise ImportProblem('Unresolved legacy investments: records have no owning clan identity; reconcile them before importing')
 index=source.read(args.index,'player_index');members=[];names=set();ids=set();listed=set();index_ended=False
 for line in index.splitlines():
  if not line.strip():continue
  if line.strip()=='~':index_ended=True;continue
  if index_ended:raise ImportProblem('Player index has trailing records')
  parts=line.split()
  if len(parts) not in (5,6):raise ImportProblem('Malformed player index row')
  id=integer(parts[0],'player ID',1);name=parts[1].lower()
  if not re.fullmatch(r'[a-z][a-z0-9]{1,19}',name) or id in ids or name in names:raise ImportProblem('Duplicate/invalid player index identity')
  ids.add(id);names.add(name);listed.add(player_path(args.players,name).resolve())
  if flags(parts[3])&1:continue
  path=player_path(args.players,name);p=player_tags(source.read(path,'players/'+path.relative_to(args.players).as_posix()))
  if integer(p.get('Id  ','0'),name+' saved ID',1)!=id or p.get('Name','').lower()!=name:raise ImportProblem(f'Player identity mismatch: {name}')
  if flags(p.get('Act ','0').split()[0])&(1<<10):raise ImportProblem(f'Player deletion flag disagrees with index: {name}')
  indexed=normalized_clan(integer(parts[5] if len(parts)==6 else '65535',name+' index clan',0,65535));saved=normalized_clan(integer(p.get('Cln ','0'),name+' saved clan',0,65535))
  if indexed!=saved:raise ImportProblem(f'Membership disagreement between index and player file: {name}')
  rank=integer(p.get('Clrk','0'),name+' rank',0,15);points=integer(p.get('CPts','0'),name+' clan points',-(1<<31),(1<<31)-1)
  if saved is None:
   if rank:raise ImportProblem(f'Rank without membership: {name}')
   continue
  members.append(dict(player_id=id,legacy_vnum=saved,rank_id=rank,legacy_points=points))
 if not index_ended:raise ImportProblem('Player index missing terminator')
 for path in Path(args.players).rglob('*.plr'):
  if path.resolve() not in listed:
   p=player_tags(source.read(path,'players/'+path.relative_to(args.players).as_posix()))
   if normalized_clan(integer(p.get('Cln ','0'),'unindexed player clan',0,65535)) is not None:raise ImportProblem(f'Unindexed clan member: {path.name}')
 return source,clans,members,claims
def make_snapshot(clans,members,claims,mapping,at):
 out=[];order=[x['vnum'] for x in clans];tag_map={tag:name for name,member,tag in FIELDS}
 known=set(tag_map)|{'vnum','Name','Init','Desc','Lder','Bank','DFnd','LAct','Rank','Priv','Ally','War'}
 for c in clans:
  if not 1<=c['vnum']<=65534 or set(c)-known:raise ImportProblem(f"Invalid vnum or unknown clan tags: {set(c)-known}")
  if not {'Name','Lder','Rank','Priv','Ally','War'}<=set(c):raise ImportProblem('Clan missing required identity/rank/privilege/relationship tags')
  uid=mapping[str(c['vnum'])];core=dict(uid=uid,vnum=c['vnum'],status=1,name=c['Name'],description=c.get('Desc',''),abbreviation=c.get('Init',''),leader_id=integer(c['Lder'],'leader ID'),treasury_gold=integer(c.get('Bank','0'),'treasury'),founded_at=integer(c.get('DFnd','0'),'founding time'),last_activity_at=integer(c.get('LAct','0'),'last activity'),created_at=at,updated_at=at,deleted_at=0,revision=0,xp_eighths=0,influence_eighths=0,state_revision=0,state_updated_at=at)
  legacy={name:integer(c.get(tag,'50' if tag=='MaxM' else '0'),tag) for tag,name in tag_map.items()};legacy['ranks']=c['Rank'];priv=[]
  for value in c['Priv']:
   pair=value.split()
   if len(pair)!=2:raise ImportProblem('Malformed privilege row')
   priv.append(dict(id=integer(pair[0],'privilege ID',0,20),rank=integer(pair[1],'privilege rank',0,255)))
  legacy['privileges']=sorted(priv,key=lambda x:x['id']);legacy['relationships']=[]
  vectors=[]
  for tag in ('Ally','War'):
   values=c[tag].split()
   if len(values)!=25:raise ImportProblem(f'{tag}: expected 25 positional flags')
   vector=[integer(x,tag,0,1) for x in values]
   if any(vector[len(order):]):raise ImportProblem('Relationship references an absent runtime clan position')
   vectors.append(vector)
  for pos,peer in enumerate(order):
   allied,war=(bool(vectors[0][pos]),bool(vectors[1][pos]))
   if allied or war:legacy['relationships'].append(dict(peer_uid=mapping[str(peer)],allied=allied,at_war=war))
  legacy['relationships'].sort(key=lambda e:e['peer_uid'])
  out.append(dict(core=core,legacy=legacy))
 converted=[]
 for member in members:
  if str(member['legacy_vnum']) not in mapping:raise ImportProblem('Membership references absent clan')
  converted.append({k:v for k,v in member.items() if k!='legacy_vnum'}|dict(clan_uid=mapping[str(member['legacy_vnum'])]))
 for entry in out:
  leader=entry['core']['leader_id'];uid=entry['core']['uid']
  if leader and not any(m['player_id']==leader and m['clan_uid']==uid and m['rank_id']==1 for m in converted):raise ImportProblem(f'Clan leader/member disagreement for vnum {entry["core"]["vnum"]}')
 claim_rows=[]
 for c in claims:
  if set(c)-{'vnum','Clan','Clmt','Popu'}:raise ImportProblem('Unknown claim tag')
  controller=normalized_clan(integer(c.get('Clan','65535'),'claim clan',0,65535))
  if controller is not None and str(controller) not in mapping:raise ImportProblem('Claim controller references absent clan')
  pop=[]
  for line in c.get('Popu',[]):
   pair=line.split()
   if len(pair)!=2:raise ImportProblem('Malformed popularity row')
   pos=integer(pair[0],'popularity runtime index',0,24)
   if pos>=len(order):raise ImportProblem('Popularity references absent runtime clan position')
   try:value=float(pair[1])
   except ValueError:raise ImportProblem('Invalid popularity amount')
   try:value=struct.unpack('f',struct.pack('f',value))[0]
   except OverflowError:raise ImportProblem('Popularity outside legacy float range')
   pop.append(dict(clan_uid=mapping[str(order[pos])],percentage=value))
  pop.sort(key=lambda e:e['clan_uid'])
  claim_rows.append(dict(zone_vnum=c['vnum'],controller_uid=mapping[str(controller)] if controller is not None else None,claimant_id=integer(c.get('Clmt','0'),'claimant ID'),popularity=pop))
 return dict(clans=sorted(out,key=lambda x:x['core']['vnum']),members=sorted(converted,key=lambda x:x['player_id']),claims=sorted(claim_rows,key=lambda x:x['zone_vnum']),written_at=at)
def differences(expected,actual):
 result=[]
 for key in ('clans','members','claims'):
  if expected[key]!=actual.get(key):result.append(key)
 return result

def main(argv=None):
 parser=argparse.ArgumentParser(description=__doc__)
 for name in ('clans','claims','index','players'):parser.add_argument('--'+name,type=Path,required=True)
 parser.add_argument('--investments',type=Path);parser.add_argument('--apply',action='store_true');parser.add_argument('--writers-quiesced',action='store_true');parser.add_argument('--backup-dir',type=Path)
 parser.add_argument('--database');parser.add_argument('--user',default='root');parser.add_argument('--host',default='');parser.add_argument('--socket',default='');parser.add_argument('--tool',type=Path)
 args=parser.parse_args(argv)
 try:
  if args.apply and (not args.database or not args.backup_dir or not args.writers_quiesced):raise ImportProblem('--apply requires --database, --backup-dir and --writers-quiesced')
  if args.apply and args.backup_dir.resolve().is_relative_to(args.players.resolve()):raise ImportProblem('Backup directory must be outside the player source tree')
  source,clans,members,claims=load_source(args);digest=source.digest();report=dict(mode='apply' if args.apply else 'dry-run',source_digest=digest,counts=dict(clans=len(clans),members=len(members),claims=len(claims)))
  with tempfile.TemporaryDirectory(prefix='clan-import-tool-') as directory:
   tool=args.tool or Path(directory)/'tool'
   if not args.tool:build(tool)
   def call(op,payload,not_found=False):
    command=[str(tool),op]+([args.database,args.user,args.host,args.socket] if op!='validate' else [])
    p=subprocess.run(command,input=json.dumps(payload,allow_nan=False),text=True,capture_output=True)
    try:out=json.loads(p.stdout)
    except (ValueError,TypeError):raise ImportProblem('Repository tool failed: '+p.stderr[:200])
    if p.returncode and not (not_found and out.get('result')=='not-found'):raise ImportProblem('Repository: '+out.get('result','unknown failure'))
    return out
   receipt=call('manifest',dict(source_digest=digest),not_found=True) if args.database else None
   existing=receipt and receipt.get('result')=='ok'
   at=receipt['imported_at'] if existing else int(time.time())
   mapping=receipt['source_manifest']['mapping'] if existing else {str(c['vnum']):secrets.token_hex(16) for c in clans}
   snapshot=make_snapshot(clans,members,claims,mapping,at);call('validate',snapshot)
   if args.database:
    actual=call('read',{});report['database_revision']=actual['store_revision']
    if existing:
     report['already_imported']=True;report['import_uid']=receipt['import_uid'];report['cut_over']=receipt['import_status']==2;report['differences']=differences(snapshot,actual)
     if args.apply and (report['cut_over'] or report['differences']):raise ImportProblem('Existing import has changed or cut over; no rows were overwritten. Use dry-run to inspect differences.')
     print(json.dumps(report,sort_keys=True));return 0
    if actual['clans'] or actual['members'] or actual['claims'] or actual['store_mode']!=0:raise ImportProblem('Target is not an empty legacy-mode staging store; no rows were overwritten')
   if args.apply:
    source.verify();report['backup']=source.backup(args.backup_dir,digest,mapping);source.verify()
    manifest=dict(source_digest=digest,import_uid=secrets.token_hex(16),imported_at=at,source_manifest=dict(files=source.digests(),source_paths={k:str(p.resolve()) for k,(p,d) in source.files.items()},mapping=mapping,backup=report['backup']))
    call('import',dict(snapshot=snapshot,expected_revision=actual['store_revision'],manifest=manifest))
    checked=call('read',{});report['differences']=differences(snapshot,checked)
    if report['differences']:raise ImportProblem('Committed import verification differs; inspect receipt before retrying')
    report['import_uid']=manifest['import_uid'];report['imported']=True
   print(json.dumps(report,sort_keys=True));return 0
 except (ImportProblem,OSError,ValueError,KeyError) as error:
  print(json.dumps(dict(error=str(error),writes='none unless a commit was already acknowledged; inspect source digest receipt before retry')),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
