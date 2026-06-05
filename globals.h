


#define MAX_STATIONS	151
#define MAX_INFO        54
#define MAX_MESSAGES    20
//#define debug
//#define rsdebug
//#define spidebug
//#define digidebug
//#define msgdebug
#define APRS_HARDWARE

//demo1 wprowadza dodatki do pelnej wersji
//#define DEMO1

//demo2 ogranicza mozliwosci programu
//#define DEMO2


#define TXQUEUELEN 8
#define TX_MESSAGEBUF 8

#define OFFLEVEL  0
#define INFOLEVEL 1
#define WARNLEVEL 2
#define DETAILLEVEL 3
#define DEBUGLEVEL 5
#define ADMINLEVEL 9



typedef struct {
   	   char sign[6];
	   char ssid;						// np. -1
	   long  dist;
	   int namiar;
	   char frame;						//typ ramki	
	   char symbol;						//symbol stacji
	   unsigned int count;						//pckts received
	   char type;						//direct=0, digi=1, it=255						//lokalizacja ramki w eeprom
	   unsigned long minutes;
	   char pathvalid;					//is path valid?
	   char path[6];					//last repeater sign
	   char info[MAX_INFO];
           char moving;       //bit 7 -moving, bits 6-0 suma position
           char position[7];  //3+3+1 (3-lat)(3-lon)(1-N/E+S/W)
           char framecrc;     //to verify duplicate frames
           char distchg;      //zbliza sie (1) czy oddala (2) nieruchomo (0)
           unsigned int slot;    //last frame in flash
	   short int wxtemp;
           char opt;          //
} station_;

typedef struct {
        unsigned int slot;
        unsigned long reptime;
        char rpflag;        //0-wolny slot 1-czeka na powtorzenie 2-powtorzona
        char alias;
        char aliaspos;
        char crc;
        char id;            //ktora to stacja?
} rpque_;

typedef struct {
        char enable;
        char alias[6];
        char aliastype;
        char hoplimit;
} _aliases;

typedef struct {
        char spath[35];
        char plen;
        char enable;
        char msgenable;
} _path;

typedef struct {
        char text[44];
        char enable;
} _status;

typedef struct {
        char stid;      //from which station
        unsigned int slot;
        char ackstatus; //0-acked/noack 1 - waiting to ack
        char status;    //0-empty 1-unread 2-read/send
        unsigned long time;
        char ackid[5];
        char option;    //0-default 1-foreign 2-bln 3-nws
} _recmessage;

typedef struct {
        unsigned int slot;
        char ackstatus;
        char repcount;
        char status;   //0 -wolny 1-zajety 2-timeout 3-transmitted
        unsigned long time;
        char ackid[5];
} _txmessage;


typedef struct {
        char data[255];
        char status;  //1-ready to send 2-busy
} _txque;

typedef struct {
        unsigned long key;
        char enable;
} _rkey;

typedef struct {
  char  text[52];
} _msgpredef;

typedef struct {
  char  alias[7];
  char  status;
  unsigned long time;
} _selfdigi;

#define NEVER   2000000000
#define NOTEMP  -127


/* zmienne dostepne wylacznie w przerwaniach */
extern char 	buffer[264];
extern char     txbuf[255];
extern char     tcpbuf[255];
extern char     msbuf[255];
extern char tcpflag;
extern _txque txqueue[TXQUEUELEN];


/* zmienne niedostepne w przerwaniach        */
extern char     napis[264];
extern char     frame[255]; //single frame buffer


/* zmienne wspoldzielone i odpowiednie flagi */
extern char traffic[60];
extern char utraffic[60]; //unique traffic
extern char dtraffic[60]; //my digi traffic
extern int  htraffic[120]; //20hr traffic

extern char gpslock;
extern char serial0_opt;

// dostep do flash
extern unsigned int spirequest;
extern unsigned int spi_buffer_to_flash_rq;
extern unsigned int spi_flash_to_rpbuf_rq;
extern unsigned int spi_txbuf_to_flash_rq;
extern unsigned int spi_flash_to_msbuf_rq;
extern unsigned int slot_lock;



extern station_ stations[MAX_STATIONS];
extern char     sort[MAX_STATIONS];
extern char 	num_stations;

#define DIGIQUEUELEN 30

extern char     rpbuf[255];
extern rpque_   rpque[DIGIQUEUELEN];

extern _rkey    rkey[4];
extern _selfdigi selfdigis[6];

/* ostatnie stacje, ktore powtorzylem */
extern _selfdigi memdigi[6];

extern _recmessage messages[MAX_MESSAGES];
extern _txmessage  txmessages[TX_MESSAGEBUF];
extern _msgpredef msgpredef[5];

extern _aliases aliases[4];
extern char digidupetime;
extern unsigned int digidelay;  //po jakim czasie powtarzac

extern _path paths[3];
extern char udest[6];
extern char pathuse;    //numer uzywanego path
extern char msgpathuse;

extern _status statusy[3];
extern char statuse;

extern char     posbank1[7];
extern char     posbank2[7];
extern char     posbank3[7];

extern char soundtab[7];

#define SND_STATION   1
#define SND_SELF      2
#define SND_CONTACT   3
#define SND_MESSAGE   4
#define SND_MARKED    5
#define SND_TRANSMIT  6

//extern unsigned long timeval;
extern unsigned char 	hangup;
extern float mylat,mylon;
//aktualny kurs stacji lub 000 jesli nie ma gps
extern int mycourse;

extern void globals_initialize(void);
extern void recalc_positions();
extern int _getopt(int opt);
extern void setopt(int opt);
extern void update_config();
extern void restore_factory();
extern void update_aprs_config();


extern char repeater_enable;
extern char txenable;
extern char queryenable;
extern char hackcheck;  //verify legal software

/* poziom uzytkownika */
extern char system_userlevel;

/* this variable indicates change in station list */
extern char receive_status;

/* #column to start display */
extern char station_list_col;

/* #pos to start list display*/
extern int station_list_start;

/* #pos to place cursor */
extern char station_list_cur;

extern char station_list_sort;

extern char update_status;

/* id of last received station */
extern char lastreceived_num;

/* czy ostatnia ramka byla unikalna */
extern char uniquemark;

extern char frame_injection;

extern unsigned int msgid;

/* zdalne operacje admina */
extern char admin_code;

/* czy mamy wazne dane z gps */
extern char gpsfix;     /* odbieram wazna pozycje z gps */
extern char gpsdata;    /* odbieram dane z gps */
extern char gpspresent; /* ta zmienna sie ustawia jesli gps przesle choc raz wazne dane */
extern char gpsenable;

extern unsigned long next_beacon_time;
extern unsigned long digisens_time;


/* odstep pomiedzu beaconami */
extern unsigned long beacon_delay;
extern char beacon_auto;
extern char beacon_digi;      //powtarza beacon jak nie wchodzi na digi

extern char navmark;

extern unsigned int packet_count;
extern unsigned int unique_packet_count;
extern unsigned int my_beacon_count;
extern unsigned int message_count;
extern unsigned int invalid_count;

extern int radar_range;

extern char filter_opt_include;
extern char filter_opt_exclude;
extern char filtered_stations;
extern char announce_filter;  //bit0 - filter by disp filter bit1 - unique
extern char self_digipeated;

extern char allmessages; //receive all messages?

extern char menup1;
extern char menup2;

extern char raxkiller;

extern char cstation;  //wskaznik do stacji na pozycji kursora
extern char marked_station;



extern unsigned char trx_txdelay;
extern unsigned char trx_txheader;
extern unsigned char trx_rxtune;
extern unsigned char trx_bitdelay;
extern unsigned char trx_squelch;

extern unsigned char msg_retries;
extern unsigned char msg_delay;

extern unsigned int sounddelay;
extern unsigned char soundmod;
extern unsigned char soundmodvar;

//my position
// 0-auto 1-gps 2-static
extern char position_mode;
extern char posuse;         //primary static

extern int rranges[5];  //lista zasiegow radaru
extern char currange;   //aktualny zasieg

extern char allframes;  //czy zapisuje wszystkie ramki
extern char tcpipframes;
extern char validonly;

#define FILTER_ALL        0x01
#define FILTER_DIRECT     0x02
#define FILTER_POSITION   0x04
#define FILTER_NOPOSITION 0x08
#define FILTER_VIADIGI    0x10
#define FILTER_WX         0x20
#define FILTER_DIGI       0x40
#define FILTER_GATE       0x80

#define UPDATE_RECEIVE  1
#define UPDATE_REFRESH  2
#define UPDATE_REDRAW   4



//options

//#define		OPT_ALLOWRP		1


extern char errtable[10];
extern char debuglevel;

extern void debugmsg(char level,char *msg);

#define SYMCNT 6
extern const char symnames[];
extern const char symtable[];

//peripherials
extern char ser_kbden;
