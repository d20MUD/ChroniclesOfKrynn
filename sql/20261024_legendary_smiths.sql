-- Historical schema-v4 compatibility step. Smiths use the Infusion Smith
-- special procedure assigned in MEDIT; no SQL registry is needed.
UPDATE legendary_schema SET version=4 WHERE singleton=1 AND version=3;
