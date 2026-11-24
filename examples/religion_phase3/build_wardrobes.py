#!/usr/bin/env python3
"""Build the reviewed Krynn wardrobe seed from authored themes and tailoring.

Run from any directory. Stable IDs occupy 300000..329999. INSERT IGNORE never
replaces a builder's saved revision. This seed is only for the Krynn campaign.
"""
import json
from pathlib import Path
here=Path(__file__).resolve().parent
THEMES=json.loads((here/'wardrobe_themes.json').read_text())
# Every rank gains visible workmanship, not magical powers or personal titles.
WORK=[
 ('plain','a single plainly worked emblem','simple seams and an unadorned edge'),
 ('corded','a corded border around the emblem','even stitching and a neatly bound edge'),
 ('bordered','a repeated emblem along the border','two parallel borders and carefully matched joins'),
 ('inlaid','an inlaid central emblem','fitted panels and a repeating inset pattern'),
 ('braided','a braided surround framing the emblem','braided edging and closely aligned panels'),
 ('filigreed','fine filigree surrounding the emblem','fine tracery worked between reinforced borders'),
 ('layered','a layered setting with paired emblems','layered borders with paired devotional panels'),
 ('intricate','an intricate setting with a complete devotional scene','a complete devotional scene running through meticulously joined panels'),
]
SLOTS=[('neck',3),('body',5),('arms',10),('legs',7),('head',6),('hands',9),('feet',8)]
NOUNS=[['symbol','robe','sleeves','trousers','cap','gloves','slippers'],
       ['pendant','leather vest','leather sleeves','leather leggings','leather cap','leather gloves','leather boots'],
       ['medallion','mail coat','mail sleeves','mail leggings','mail coif','mail gauntlets','mail boots'],
       ['gorget emblem','plate cuirass','plate armguards','plate greaves','plate helm','plate gauntlets','plate sabatons']]
CONSTRUCTION=['Woven fabric carries the pattern in dyed thread.',
 'The pattern follows the seams of supple leather, leaving the joints free.',
 'Small decorative links trace the pattern across the mail without obscuring its construction.',
 'The pattern is engraved into the plate borders, with riveted accents at each join.']
def entries():
 for t in THEMES:
  for rank,(adjective,emblem,detail) in enumerate(WORK,1):
   for category in range(4):
    for index,(slot,pos) in enumerate(SLOTS):
     ident=300000+t['id']*1000+rank*100+category*10+index
     noun=NOUNS[category][index]
     short=f"a {adjective} {t['name']} {noun}"
     inspection=(f"The {noun} is finished in {t['colors']}. It bears {t['symbol']}, with {t['motif']} worked into the design. "
                 +(f"A setting of {t['material']} presents {emblem}." if slot=='neck' else CONSTRUCTION[category]+f" Its workmanship shows {detail}.")
                 +f" This is the rank-{rank} {['clothing','light','medium','heavy'][category]} appearance of {t['name']}'s church. Its decoration conveys no personal office.")
     yield {'id':ident,'category':2,'org':t['id'],'rank':rank,'position':pos,
            'kind':'symbol' if slot=='neck' else 'armor','value':category,
            'variant':['clothing','light','medium','heavy'][category],
            'deity_name':t['name'],'campaign':'DL',
            'keywords':f"{t['name'].lower()} {noun} {adjective}",
            'short':short,'room':short[0].upper()+short[1:]+" lies here.",
            'inspection':inspection}
def quote(s):
 # Hex literals are independent of server SQL backslash-escape mode.
 return 'CONVERT(0x'+s.encode().hex()+" USING utf8mb4)"
def main():
 rows=list(entries())
 assert len(rows)==21*8*4*7 and len({r['id'] for r in rows})==len(rows)
 out=['-- Krynn ONLY. Apply 20261123_religion_phase3.sql first.',
      '-- Authored text seed; does not configure temples or change existing entries.',
      'START TRANSACTION;']
 for r in rows:
  data=json.dumps(r,separators=(',',':'))
  out.append(f"INSERT IGNORE INTO restring_catalogue(id,revision,published) VALUES({r['id']},1,{quote(data)});")
 out.append('COMMIT;')
 (here/'wardrobes.sql').write_text('\n'.join(out)+'\n')
 print(f'Wrote {len(rows)} appearances: 224 per deity, 28 per rank.')
if __name__=='__main__': main()
