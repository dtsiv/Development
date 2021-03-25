SELECT f.filever,s.id AS guid, s.seqnum, 
s.ncnt, f.timestamp, s.structStrobData,
sig.filedata, sig.sigtype, sig.samplcnt,
nm.id AS noisemapid, nm.nfft, nm.ifilteredn FROM
strobs s LEFT OUTER JOIN files f ON f.id=s.fileid
LEFT OUTER JOIN signals sig ON s.sigid=sig.id
LEFT OUTER JOIN noisemaps nm ON s.noisemapid=nm.id
ORDER BY s.seqnum ASC
					  
					  
					  
SELECT nm.id AS noisemapid 
FROM noisemaps nm LEFT OUTER JOIN 
strobs st ON nm.id=st.noisemapid 
LEFT OUTER JOIN files f 
ON f.id=st.fileid AND f.id<>:fileid 
WHERE f.id IS NULL



SELECT DISTINCT tbl.nmid, fi.id AS fiid FROM (SELECT nm.id AS nmid, f.id AS fid FROM 
noisemaps nm INNER JOIN strobs st ON st.noisemapid=nm.id
INNER JOIN files f ON f.id=st.fileid) tbl
LEFT OUTER JOIN files fi ON fi.id=tbl.fid AND fi.id<>18
WHERE fiid IS NULL


SELECT DISTINCT nm1.id FROM noisemaps nm1
LEFT OUTER JOIN (SELECT DISTINCT nm.id AS nmid, f.id AS fid FROM 
noisemaps nm INNER JOIN strobs st ON st.noisemapid=nm.id
INNER JOIN files f ON f.id=st.fileid) tbl ON tbl.nmid=nm1.id AND tbl.fid<>17
WHERE tbl.fid IS NULL


SELECT DISTINCT si1.id FROM signals si1
LEFT OUTER JOIN (SELECT DISTINCT si.id AS siid, f.id AS fid FROM signals si
INNER JOIN strobs st ON st.sigid=si.id
INNER JOIN files f ON f.id=st.fileid) tbl ON tbl.siid=si1.id AND tbl.fid<>:fileid
WHERE tbl.fid IS NULL