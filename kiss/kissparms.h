#define PARAM_TXDELAY   1
#define PARAM_PERSIST   2
#define PARAM_SLOTTIME  3
#define PARAM_TXTAIL    4
#define PARAM_FULLDUP   5
#define PARAM_HARDWARE  6
#define PARAM_MUTE      7
#define PARAM_FECLEVEL  8 /* linux kernel only supports FECLEVEL for special modems */
#define PARAM_DTR       8
#define PARAM_RTS       9
#define PARAM_SPEED     10
#define PARAM_ENDDELAY  11
#define PARAM_GROUP     12
#define PARAM_ACKMODE   12 /* an other dialect */
#define PARAM_ECHO      12 /* an other dialect */
#define PARAM_IDLE      13
#define PARAM_RXDELAY   13 /* an other dialect */
#define PARAM_MIN       14
#define PARAM_MAXKEY    15
#define PARAM_RETURN    0xFF


/* non-standard commands > 0x0f are best interpreted localy.
   Be aware that "multiport kiss" is incompatible to crc.
   CRC: smack is 0x80, flexnet is 0x20
   xxxxyyyy:
       yyyy are the kiss commndas 0x00-0x0f (0x0 == data)
   1000     is CRC smack
   0010     is CRC flexnet
   xxxx     is multiport address (TNC1-16) where the command / data goes to

   I.e. 0x10 PARAM_WAIT may conflict with data franme for second TNC.
   I.e. 0x20 could mean: data with crc-flexnet, vs data to TNC #3.
   I.e. 0x80 could mean: data with crc-smack, vs data to TNC #9.
   I.e. 0x81 could mean: local command versus "set TX delay for TNC #9".
                         Theoreticaly also crc-smack signed txdelay commad for
                         TNC1.
                         In practise: not the case, because it's defined
                         that kiss commands have no CRC. Else PARAM_DCD
                         response would be interpreted as crc-smack signed
                         TXTAIL command, or PARAM_RETURN as crc-smack signed
                         cmd 0x0f.
                         -> In the world of CRC, 0x81 - 0x8f or 0x21-0x2f are
                         forbidden, because if would imply command with
                         appended CRC.
   => Only kiss commands 0x01 - 0x015 and 0xff are safe.
 */

#define PARAM_WAIT      0x10
#define PARAM_DOWN      0x81 /* wampes: close interface port */
#define PARAM_UP        0x82 /* wampes: (re)open interface port */
#define PARAM_BLIND     0x83
#define PARAM_DCD       0x84 /* do be verified. read parameter for TNC's DCD? */
#define PARAM_CRC       0x85 /* linux kernel-ax25 internal command for setting
                              * CRC mode (auto, smack, flexnet, none) */
