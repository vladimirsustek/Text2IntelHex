Program creates an intel-8-hex file like:
:100000000C9434000C9451000C9451000C94510049<CR><LF>
:100010000C9451000C9451000C9451000C9451001C<CR><LF>
..down to..<CR><LF>
:00000001FF<CR><LF>
Writes to alligned 16-data bytes lines (20 total),
and is capable only to write data (type 0x00) and terminate them (0x01).
Program expects two arguments:
1) path to text file XXX.txt with any string terminated by <EOF>.
2) address where should be record stored (only 0x0000 - 0x01FF).
Multiple records with different addresses not supported.
To be called like "Text2IntelHex myFile.txt 0000 > myHex.eep".