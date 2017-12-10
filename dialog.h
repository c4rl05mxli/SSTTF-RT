#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QtSerialPort/QSerialPort>
#include <QByteArray>
#include <QGraphicsScene>
#include <QVector>
#include <QDate>
#include <QTime>



namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();


private slots:
    void readSerialGPS();
    void readSerialTIC();
    void update_Data();
    void sendcommand(unsigned char*,quint16,quint16);
    //GPS Parse
    void Ha_Data(QByteArray Ha);
    void Hn_Data(QByteArray Hn);
    void Bb_Data(QByteArray Bb);
    //TIC Functions
    void TIC_parse_data();


    //Secuencia de inicio del GPS
    void SettoDefaults();
    void TimeMessage();
    void ReceiverID();   // sin uso actualmente, no se envia
    void PPSOneSat();   // desactivo el pulso por segundo
    void SurveyPosition();
    void SendPosition();
    void SendPositionHold();
    void EnableTRAIM();
    void StartPolling();    // aqui no se hace un polling, se recibe automaticamente cada segundo el triple command
    void StopPolling();
    void ReceiverStatus(quint16);
    void ReadSettings();
    //Secuencia especial para M8T
    void SetConstellationMode();
    void SetDateTimePPSAlignment();





    void SavePosition();



    void UpdateUTC();
    quint32 JulianDate(QDate);
    void LLAtoECEF(double,double,double);

    void SaveData();



    //Buttons
    void on_btnSurvey_clicked();
    void on_btnReset_clicked();
    void on_btnSetPHold_clicked();
    void on_btnEnableTRAIM_clicked();
    void on_btnEnablePPS_clicked();
    void on_btnConfig_clicked();
    void on_btnStartPoll_clicked();
    void on_btnStopPoll_clicked();
    void on_btnSavePosition_clicked();



private:
    Ui::Dialog *ui;
    QSerialPort *GPS;
    static const quint16 GPS_vid = 1659;//17224;       // Constants for automatic identification
    static const quint16 GPS_pid = 8963;//21795;
    QByteArray serialData;
    QByteArray HnData;
    QString serialBuffer;
    QString parsed_data;

    double data_value;

    enum GPS
    {
       // PRN,dBm,TD,DT,AZTH,ELV
        PRN,dBm,AZTH,ELV
    };

    enum CMD
    {
        Reset,GPS_ID,Gd,Ha,Hn,Bb
    };

    //TIC Variables
    QSerialPort *TIC;
    static const quint16 TIC_vid = 9025;
    static const quint16 TIC_pid = 66;
    QByteArray TIC_Data;
    QString TIC_Buffer;
    QString TIC_parsed_data;
    double TIC_data_value;

    //Plot
    QVector<double> qv_x,qv_y;
    //SatelliteData
    double satclocks[33][4];    // used to store and count satellite offsets
    qint32 prn[33][3];          // store time offset, used in time solution, elevation angle

    //Path variables
    QString path="";
    QString config_path="";
    QString data_path="";

    QString status="";
   //Ha
   //Global Variables Declaration
    qint8   negative_sawtooth;
    qint8   last_negative_sawtooth;
    QDate   GPSdate;
    QTime   GPStime;
    QByteArray rawpos;
    qint32  latitudAU;
    double  latitud=0.0;
    qint32  longitudAU;
    double  longitud=0.0;
    qint32  heightAU;
    double  altitud=0.0;
    qint8   nvs;   //Number of visible satellites  0...12
    qint8   nts;   //Number of tracked satellites  0...12
    QString ID;
    quint16 receiver_status=0;

    //Hn commands
    quint8  pulse_status;
    quint8  PPS_sync;
    quint8  TimeRAIM_solution;
    quint8  TimeRAIM_status;
    quint32 TimeRAIM_svid;
    quint16 time_solution;
   //Satellites Structure
    //Satellites
    struct{
        //Ha
        quint8     mode=0;
        quint8     signal=0;
        quint8     IODE=0;
        quint16    status=0;
        //Hn
        quint32    fractional_GPS_time=0;  //-5000...5000
        //Bb
        qint16     Doppler=0;   //-5000...5000
        quint8     elevation=0; //0...90
        quint16    azimuth=0;   //0...359
        qint8      sat_health=0;//0 healthy and not removed; 1 unhealthy and removed
    }SVID[33];

    //Offsets
   quint8  inview=0;
   qint64  sumoffset=0;
   double  meanoffset=0; // se cambia a double
   double  satdiff=0.0;


   //TIC Variables
   quint64 samples=0;
   double  count=0.0;
   double  lastcount=0.0;
   double  maxcount=0.0;
   double  mincount=0.0;
   double  range=0.0;
   double  midpoint=0.0;
   double  csum = 0.0;
   double  mean=0.0;
   double  meancount=0.0;
   double  sumcount=0.0;
   double  sq=0.0;
   double  sumx2=0.0;
   double  sd=0.0;
   double  diffcount=0.0;

   //flags
   bool CMD_Ha=false;
   bool CMD_Hn=false;
   bool CMD_Bb=false;




   //Network variables
   QString IPv4="";
   QString SubMask="";
   QString Gateway="";


   //returnString
   quint16 response_lenght=0;
   bool    response_ready=false;

   //Settings
   QString settings[10];

   //ECEF
   double ECEF_X;
   double ECEF_Y;
   double ECEF_Z;

   //Sky Plot Variables
   QGraphicsScene *skyplot;
   QGraphicsTextItem *skyplot_text;

   QBrush *brush_white;
   QBrush *brush_yellow;
   QBrush *brush_black;
   QBrush *brush_blue;
   QPen *pen_black;

   void skyplot_init();
   void skyplot_add(quint8 el, quint16 azi, quint8 prn);
   void skyplot_clear();


   //CustomPlot

  // QCustomPlot *customPlot;
   void customPlot_init();
   void customPlot_add(QTime t,int x, int y);
   void customPlot_clear();


};

#endif // DIALOG_H
