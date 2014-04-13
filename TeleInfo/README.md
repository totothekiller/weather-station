Teleinformation
========================

Connection between Arduino / ATtiny85 and electricy meter (EDF compteur type A14C5)

Parts :
* ATtiny85
* Optocoupler SFH620A 
* RF transmitter 434 MHz

[Arduino Sketch](Tx_Node_TeleInfo/Tx_Node_TeleInfo.ino)

Inspiration and original idea from http://www.planet-libre.org/index.php?post_id=11122


EDF Segment structure
------------------------

| Prefix     | Description                     | 
| ---------- | ------------------------------- |
| PAPP	     | Instant Power                   |
| HCHC	     | Compteur Heure Creuse           |
| HCHP	     | Compteur Heure Pleine           |


Example :
```
__
ADCO 000000000000 B
OPTARIF HC.. <
ISOUSC 30 9
HCHC 003640993 (
HCHP 002633434 ,
PTEC HP..  
IINST 002 Y
IMAX 036 H
PAPP 00460 +
HHPHC A ,
MOTDETAT 000000 B
__
ADCO 000000000000 B
OPTARIF HC.. <
ISOUSC 30 9
HCHC 003640993 (
HCHP 002633434 ,
PTEC HP..  
IINST 002 Y
IMAX 036 H
PAPP 00450 *
HHPHC A ,
MOTDETAT 000000 B
__
ADCO 000000000000 B
OPTARIF HC.. <
ISOUSC 30 9
HCHC 003640993 (
HCHP 002633434 ,
PTEC HP..  
IINST 002 Y
IMAX 036 H
PAPP 00460 +
HHPHC A ,
MOTDETAT 000000 B
__
```

