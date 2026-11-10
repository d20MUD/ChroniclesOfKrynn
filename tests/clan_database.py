"""Private MariaDB fixture; no game configuration or network connections."""
from contextlib import contextmanager
from pathlib import Path
import getpass,subprocess,tempfile,time
ROOT=Path(__file__).resolve().parents[1]
@contextmanager
def database(migrations=('20261110_clan_progression_v001.sql',)):
 with tempfile.TemporaryDirectory(prefix='clan-db-') as directory:
  tmp=Path(directory);sock=tmp/'mysql.sock';log=tmp/'server.log'
  subprocess.run(['mariadb-install-db','--no-defaults','--datadir='+str(tmp/'data'),'--auth-root-authentication-method=normal','--skip-test-db'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
  process=subprocess.Popen(['mariadbd','--no-defaults','--datadir='+str(tmp/'data'),'--socket='+str(sock),'--pid-file='+str(tmp/'pid'),'--skip-networking','--log-error='+str(log),'--user='+getpass.getuser()],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  client=['mariadb','--no-defaults','--socket='+str(sock),'-uroot','--batch','--skip-column-names']
  try:
   for _ in range(100):
    if subprocess.run([*client,'-e','SELECT 1'],capture_output=True).returncode==0:break
    if process.poll() is not None:raise RuntimeError(log.read_text())
    time.sleep(.1)
   else:raise RuntimeError('Temporary database startup failed')
   subprocess.run([*client,'-e','CREATE DATABASE clan_test CHARACTER SET utf8mb4'],check=True)
   client.append('clan_test')
   def sql(text):return subprocess.run(client,input=text,text=True,capture_output=True,check=True).stdout.strip()
   for migration in migrations:sql((ROOT/'sql'/migration).read_text())
   yield tmp,sock,sql,client
  finally:
   process.terminate()
   try:process.wait(timeout=15)
   except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
