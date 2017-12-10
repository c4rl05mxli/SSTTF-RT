#include "dialog.h"
#include "ui_dialog.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QMessageBox>
#include <QtCore>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QNetworkInterface>


QString namefile="";
double int_dly=0.0;
double cbl_dly=0.0;
double ref_dly=0.0;

QDate   Local_date;
QTime   Local_time;




Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    //GPS Table init
    QStringList GPScol,GPSrow;
    GPScol << "PRN" << "dBm" << "AZTH" << "ELV";
    GPSrow << "CH01"<<"CH02"<<"CH03"<<"CH04"<<"CH05"<<"CH06"<<
              "CH07"<<"CH08"<<"CH09"<<"CH10"<<"CH11"<<"CH12";
    ui->tableWidget->setColumnCount(GPScol.size());
    ui->tableWidget->setHorizontalHeaderLabels(GPScol);
    ui->tableWidget->setRowCount(GPSrow.size());
    ui->tableWidget->setVerticalHeaderLabels(GPSrow);

    GPS= new QSerialPort(this); //creo un nuevo objeto de puerto serial
    serialBuffer ="";
    serialData="";
    data_value=0.0;     // inicializo variables

    TIC= new QSerialPort(this); //creo un nuevo objeto de puerto serial
    TIC_Buffer="";
    TIC_parsed_data="";
    TIC_data_value=0.0;

/*
//      Testing code, prints the description, vendor id, and product id of all ports.
//       Used it to determine the values of GPS and TIC.
    qDebug() << "Number of ports: " << QSerialPortInfo::availablePorts().length() << "\n";
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
        qDebug() << "Description: " << serialPortInfo.description() << "\n";
        qDebug() << "Has vendor id?: " << serialPortInfo.hasVendorIdentifier() << "\n";
        qDebug() << "Vendor ID: " << serialPortInfo.vendorIdentifier() << "\n";
        qDebug() << "Has product id?: " << serialPortInfo.hasProductIdentifier() << "\n";
        qDebug() << "Product ID: " << serialPortInfo.productIdentifier() << "\n";
    }
*/

       /*
        *   Identify the ports
        */
       bool GPS_is_available = false;
       QString GPS_port_name;
       bool TIC_is_available = false;
       QString TIC_port_name;
       //
       //  For each available serial port
       foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
           //  check if the serialport has both a product identifier and a vendor identifier
           if(serialPortInfo.hasProductIdentifier() && serialPortInfo.hasVendorIdentifier()){
               //  check if the product ID and the vendor ID match those of the GPS
               if((serialPortInfo.productIdentifier() == GPS_pid)
                       && (serialPortInfo.vendorIdentifier() == GPS_vid)){
                   GPS_is_available = true; //    TIC is available on this port
                   GPS_port_name = serialPortInfo.portName();
               }
               if((serialPortInfo.productIdentifier() == TIC_pid)
                       && (serialPortInfo.vendorIdentifier() == TIC_vid)){
                   TIC_is_available = true; //    TIC is available on this port
                   TIC_port_name = serialPortInfo.portName();
               }


           }
       }

       /*
        *  Open and configure the GPS port if available
        */
       if(GPS_is_available){
           //Serial Port GPS
           qDebug() << "Found the GPS" << GPS_port_name << endl;
           GPS->setPortName(GPS_port_name);
           GPS->open(QSerialPort::ReadWrite); //Read and Write Device, Unbuffered
           GPS->setBaudRate(QSerialPort::Baud9600);
           GPS->setDataBits(QSerialPort::Data8);
           GPS->setFlowControl(QSerialPort::NoFlowControl);
           GPS->setParity(QSerialPort::NoParity);
           GPS->setStopBits(QSerialPort::OneStop);
           QObject::connect(GPS, SIGNAL(readyRead()), this, SLOT(readSerialGPS()));
       }else{
           qDebug() << "Couldn't find the correct port for the GPS.\n";
       }

       /*
        *  Open and configure the TIC port if available
        */
       if(TIC_is_available){
            //Serial Port TIC
            qDebug() << "Found the TIC" << TIC_port_name << endl;
            TIC->setPortName(TIC_port_name);
            TIC->open(QSerialPort::ReadOnly); //Read and Write Device, Unbuffered
            TIC->setBaudRate(QSerialPort::Baud115200);
            TIC->setDataBits(QSerialPort::Data8);
            TIC->setFlowControl(QSerialPort::NoFlowControl);
            TIC->setParity(QSerialPort::NoParity);
            TIC->setStopBits(QSerialPort::OneStop);
            QObject::connect(TIC, SIGNAL(readyRead()), this, SLOT(readSerialTIC()));
       }else{
           qDebug() << "Couldn't find the correct port for the TIC.\n";
       }


       QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
       for(int i=0; i<interfaces.count(); i++)
       {
            QList<QNetworkAddressEntry> entries = interfaces.at(i).addressEntries();
                for(int j=0; j<entries.count(); j++)
                {

                    if(entries.at(j).ip().protocol() == QAbstractSocket::IPv4Protocol)
                    {
                       if (entries.at(j).ip().toString() != "127.0.0.1")
                        {

                            IPv4=entries.at(j).ip().toString();
                            SubMask=entries.at(j).netmask().toString();
                            Gateway=entries.at(j).broadcast().toString();

                            qDebug() << "Interface [" << QString::number(i+1) << "]" << endl;
                            qDebug() << "IP:" << IPv4 << endl;
                            qDebug() << "MASK" << SubMask << endl;
                            qDebug() << "Gateway" << Gateway <<endl;

                        }
                     }

                }
       }

// Directory Creation
        path = QCoreApplication::applicationDirPath(); //working directory to path variable
        qDebug() << path << endl;
        config_path=path + "/config";
        data_path=path + "/data" ;

        QDir  config(config_path);

        if (config.exists())
        {
            qDebug() << "true" << config << endl;
        }
        else
        {
            qDebug() << "false" << config << endl;
            config.mkdir(config_path);
        }
        QDir  data(data_path);
        if (data.exists())
        {
            qDebug() << "true" << data << endl;
        }
        else
        {
            qDebug() << "false" << data << endl;
            data.mkdir(data_path);
        }

// Log file creation
QFile file(config_path + "/log.txt");

if (!file.open(QFile::WriteOnly | QFile::Append))
{
    qDebug() << "File not open" <<endl;
}
    QTextStream out(&file);
    QString text= "Application Started";
    out << QDate::currentDate().toString(Qt::ISODate) <<"/" << QTime::currentTime().toString(Qt::ISODate) << " " << text << endl;
    file.flush();
    file.close();
    //
    ReadSettings(); //leo dos veces, la primera lo creo si no existe
    ReadSettings();
}

Dialog::~Dialog()
{
    if(GPS->isOpen()){
           GPS->close(); //    Close the serial port if it's open.
       }
    if(TIC->isOpen()){
           TIC->close(); //    Close the serial port if it's open.
       }
    delete ui;
}

void Dialog::readSerialGPS()
{

    QByteArray _STX("@@");
    QByteArray _ETX("\r\n");
    QByteArray Ha("Ha");
    QByteArray Hn("Hn");
    QByteArray Bb("Bb");
    QByteArray Cb("Cb"); //almanac

    // Respuestas de la secuencia de inicio del GPS
    QByteArray Cf("Cf");    // Set to Defaults
    QByteArray Sc("Sc");    // Set Constellation Mode to GPS Only
    QByteArray St("St");    // Set sets time & 1PPS alignment source to UTC(USNO)

    QByteArray Gb("Gb");    // Set DateTime to GPS   // DEsactivado
    QByteArray Gc("Gc");    // Set PPS to one sat only
    QByteArray Ch("Ch");    // Almanac input
    QByteArray As("As");    // Position Hold input
    QByteArray Gd("Gd");    // Set Position Hold
    QByteArray Ay("Ay");    // Time Offset Command
    QByteArray Az("Az");    // Cable Delay Correction
    QByteArray Ge("Ge");    // Set Enable T-RAIM


    QByteArray  _CMD;
    qint16     STX=0;
    qint16     ETX=0;
    quint16 dataSize = 0;
    quint16 cmd_lenght=0;

    dataSize = GPS->bytesAvailable(); //determino numero de caracteres en el puerto
   //qDebug() <<   "GPS Bytes Available: " << dataSize << "\n";
    serialData = serialData + GPS-> read(dataSize); //leo el numero de caracteres
   //qDebug() << serialData <<endl;
   //qDebug() << serialData.size() << endl;



    if (response_lenght==0)
    {
        if(serialData.size()>=6)
        {
           //Find STX (@@)
           STX = serialData.indexOf(_STX,0);
           if(STX >= 0)
           {
        //   qDebug()    <<  "Index of STX"  <<  STX;
           _CMD=serialData.mid(STX+2,2);
          // qDebug()  << "CMD: " << _CMD  ;

           if (_CMD==Ha)
           {
               cmd_lenght=154;
           }

           if (_CMD==Hn)
           {
               cmd_lenght=78;
           }

           if (_CMD==Bb)
           {
               cmd_lenght=92;
           }



           ETX = serialData.indexOf(_ETX,STX+cmd_lenght-4);


       //     qDebug()    <<  "Index of ETX"  << ETX;

           }




         //   qDebug()    <<  "Index of ETX"  <<  serialData.indexOf(_ETX,0);






              if (ETX > 0)
              {
                  if (_CMD==Ha)
                  {
                  Ha_Data(serialData.mid(STX,ETX+2));
                  serialData.remove(STX,ETX+2);
                  CMD_Ha=true;
                    //Dialog::update_Data();
                  }

                  if (_CMD==Hn)
                  {
                  Hn_Data(serialData.mid(STX,ETX+2));
                  serialData.remove(STX,ETX+2);
                  CMD_Hn=true;
                    //Dialog::update_Data();
                  }

                  if (_CMD==Bb)
                  {
                  Bb_Data(serialData.mid(STX,ETX+2));
                  serialData.remove(STX,ETX+2);
                  CMD_Bb=true;
                   // Dialog::update_Data();
                  }

                  if(CMD_Ha & CMD_Hn & CMD_Bb)
                  {
                   Dialog::update_Data();
                   CMD_Ha=false;
                   CMD_Hn=false;
                   CMD_Bb=false;
                  }
              }






        }

    }
    else if (serialData.size()>=response_lenght)
    {
     qDebug() << "[" << serialData.size() << "] = " << serialData << endl;



         STX = serialData.indexOf(_STX,0);
         if(STX >= 0)
         {
             _CMD=serialData.mid(STX+2,2);
            //Aqui defino la secuencia de comandos iniciales, esto es muy mal hecho y necesita cambiarse
             if (_CMD==Cf){
                  qDebug() << "GPS Set to Defaults" << endl;
                  serialData.clear();
             }

             if (_CMD==Sc){
                 qDebug() << "Constallation Set to GPS Only" << endl;
                 SetDateTimePPSAlignment();
             }

             if (_CMD==St){
                 qDebug() << "Time & 1PPS aligned to UTC(USNO)" << endl;
                TimeMessage();
             }

             if (_CMD==Gb){
                  qDebug() << "GPS Set Date and Time" << endl;
                   PPSOneSat();
             }
             if (_CMD==Gc){
                  qDebug() << "1PPS Control Message Set to active only when tracking at least one satellite" << endl;

             }
             if (_CMD==Ch){
                  qDebug() << "Almanac Data Input" << endl;
                 SendPosition();
             }
             if (_CMD==As){
                  qDebug() << "Position to Hold Input" << endl;
                SendPositionHold();
             }
             if (_CMD==Gd){
                  qDebug() << "Position Hold Mode" << endl;

             }
             if (_CMD==Ay){
                 qDebug()<< "Advanced 1PPS 5ms" << endl;
                // Cable_delay();
             }
             if (_CMD==Az){
                 //qDebug()<< "Cable delay corrected " << QString::number(cable_delay,10) << " ns" << endl;
                 EnableTRAIM();
             }
             if (_CMD==Ge){
                  qDebug() << "T-RAIM Enabled" << endl;
                  StartPolling();
                  qDebug() << "GPS Started" << endl;
             }

         }

     response_ready=true;
     serialData.clear();
    }
}

void Dialog::readSerialTIC()
{
     quint16 dataSize = 0;
     qint16     STX=0;
     qint16     ETX=0;

     dataSize = TIC->bytesAvailable(); //determino numero de caracteres en el puerto
    // qDebug() <<   "Bytes Available: " << dataSize << "\n";
     TIC_Data = TIC_Data + TIC-> read(dataSize); //leo el numero de caracteres
    // qDebug() << TIC_Data << endl;

     if(TIC_Data.count()>=18)
       {
           STX = TIC_Data.indexOf(0x02,0);
           if(STX >= 0)
           {
    //        qDebug()    <<  "Index of STX"  <<  STX;


           ETX = TIC_Data.indexOf(0x03,STX);

          if(ETX>=0)
          {

      //       qDebug()    <<  "Index of ETX"  <<  ETX;

          TIC_Buffer=TIC_Data.mid(STX+1,(ETX-STX)-1);
          count=TIC_Buffer.toDouble();

//          if (count<1)
//          {
//              count=1.0-count;
//          }
          count=count*1e9;
          count=count;//+last_negative_sawtooth+int_dly+cbl_dly+ref_dly;
          qDebug() << GPStime.toString("hhmmss") << "," << QString::number(count,'f',2) << "," << QString::number(negative_sawtooth);

               //  << QString::number(count-negative_sawtooth,'f',0);
//          qDebug() << "SawTooth" << QString::number(negative_sawtooth,'f',2);
//          qDebug() << "C-" << QString::number(count-negative_sawtooth,'f',2);
//          qDebug() << "C+" << QString::number(count+negative_sawtooth,'f',2);
//          qDebug() << "LSawTooth" << QString::number(last_negative_sawtooth,'f',2);
//          qDebug() << "LC-" << QString::number(count-last_negative_sawtooth,'f',2);
//          qDebug() << "LC+" << QString::number(count+last_negative_sawtooth,'f',2);
          TIC_parse_data();
        //  qDebug() << TIC_Buffer;
          TIC_Data.remove(STX,ETX);

           }
           }


       }


}

void Dialog::update_Data()
{
    UpdateUTC();

    //Position
    ui->lneLatitude->setText(QString::number(latitud, 'g', 10));
    ui->lneLongitude->setText(QString::number(longitud, 'g', 10));
    ui->lneAltitude->setText(QString::number(altitud, 'g', 10));
    //Satellite
    ui->lneSVT->setText(QString::number(nvs,10) + "/" + QString::number(nts,10));
    ui->lneSawtooth->setText(QString::number(negative_sawtooth,10));
    ui->lneStatus->setText(status);


    ui->lcdLast->display(QString::number(count,'f',2));
    ui->lcdMean->display(QString::number(mean,'f',2));

    ui->tableWidget->clearContents();   //CLear GPS table before write again

 int channel=0;
    for(int i=1 ; i <=32 ; i++)
    {
        if (SVID[i].mode ==8)
        {
        double snr=20*(qLn(SVID[i].signal)/qLn(10))-163.4;
        //GUI
        ui->tableWidget->setItem(channel,PRN,new QTableWidgetItem(QString::number(i)));
        ui->tableWidget->setItem(channel,dBm,new QTableWidgetItem(QString::number(snr,'f',2)));
        ui->tableWidget->setItem(channel,AZTH,new QTableWidgetItem(QString::number(SVID[i].azimuth, 10)));
        ui->tableWidget->setItem(channel,ELV,new QTableWidgetItem(QString::number(SVID[i].elevation, 10)));
        channel=channel+1;
       }

    }

}

void Dialog::Ha_Data(QByteArray Ha)
{

//154 Bytes

    quint8 SAT_ID;
    // Start @ @ H a Data:   0,1,2,3
    // Date  m d y y Data:   4,5,6,7
    GPSdate.setDate((Ha.at(6)<<8 & 0xFF00) | (Ha.at(7) & 0x00FF),Ha.at(4),Ha.at(5));
    // Time h m s    Data:   8,9,10
    GPStime.setHMS(Ha.at(8),Ha.at(9),Ha.at(10),0);
    // nanoSeconds   Data:   11,12,13,14
    ///////////////////////////////////
    // Latitude FS   Data:   15,16,17,18
    // Longitud FS   Data:   19,20,21,22
    // GPS Height FS Data:   23,24,25,26
    // MSL Height FS Data:   27,28,29,30
    ///////////////////////////////////
    // Latitud AU    Data:   31,32,33,34
    rawpos.resize(12);
    latitudAU= (Ha.at(31) << 24 & 0xFF000000) | (Ha.at(32) << 16 & 0x00FF0000) | (Ha.at(33) << 8 & 0x0000FF00)| (Ha.at(34) & 0x000000FF);
    rawpos[0]=Ha.at(31);
    rawpos[1]=Ha.at(32);
    rawpos[2]=Ha.at(33);
    rawpos[3]=Ha.at(34);
    latitud=(double)latitudAU/3600000;
    // Longitud AU   Data:   35,36,37,38
    longitudAU=(Ha.at(35) << 24 & 0xFF000000) | (Ha.at(36) << 16 & 0x00FF0000) | (Ha.at(37) << 8 & 0x0000FF00)| (Ha.at(38) & 0x000000FF);
    rawpos[4]=Ha.at(35);
    rawpos[5]=Ha.at(36);
    rawpos[6]=Ha.at(37);
    rawpos[7]=Ha.at(38);
    longitud=(double)longitudAU/3600000;
    // GPS Height AU Data:   39,40,41,42
    heightAU=(Ha.at(39) << 24 & 0xFF000000) | (Ha.at(40) << 16 & 0x00FF0000) | (Ha.at(41) << 8 & 0x0000FF00)| (Ha.at(42) & 0x000000FF);
    rawpos[8]=Ha.at(39);
    rawpos[9]=Ha.at(40);
    rawpos[10]=Ha.at(41);
    rawpos[11]=Ha.at(42);
    altitud = (double)heightAU/100;
    // MSL Height AU Data:   43,44,45,46
    // Speed 3D      Data:   47,48
    // Speed 2D      Data:   49,50
    // Heading 2D    Data:   51,52
    // Geometry      Data:   53,54
    // Satellite     Data:   55,56
    nvs=Ha.at(55);     //Number of visible satellites  0...12
    nts=Ha.at(56);     //Number of tracked satellites  0...12
    //Satellites    Data:   57-128
    for(int i=0 ; i <12 ; i++)
    {
         SAT_ID=Ha.at(57+i*6); //63
         //Validar que solo vea GPS
         if (SAT_ID < 33){
         SVID[SAT_ID].mode=Ha.at(58+i*6); //64
         SVID[SAT_ID].signal=Ha.at(59+i*6);
         SVID[SAT_ID].IODE=Ha.at(60+i*6);
         SVID[SAT_ID].status=(Ha.at(61+i*6)<<8 & 0xFF00) | (Ha.at(62+i*6) & 0x00FF);
      /*  qDebug() << "SatNum"  << QString::number(i+1, 10) <<  QString::number(SVID[i].mode, 10) <<
                      QString::number(SVID[i].signal, 10) <<
                       QString::number(SVID[i].IODE, 10) << endl;*/
    }
    }


    // Receiver Status ss:   129,130
    receiver_status= (Ha.at(129)<<8 & 0xFF00) | (Ha.at(130) & 0x00FF);
    ReceiverStatus(receiver_status);
    // Reserved      Data:   131,132
    // Oscillator and Clock Parameters
    // Clock Bias cc Data:   133,134
    // OscOffset ooooData:   135,136,137,138
    // Temperature   Data:   139,140
    // TimeModeUTC u Data:   141
    // GMTOffset shm Data:   142,143,144
    // IDstring      Data:   145,146,147,148,149,150
    ID.append(Ha.at(145));ID.append(Ha.at(146));ID.append(Ha.at(147));ID.append(Ha.at(148));ID.append(Ha.at(149));ID.append(Ha.at(150));
    // Stop  C,CR,LF Data:   151,152,153
    // Print Debugs
    // qDebug() << "Ha Command Raw Data"   << Ha.size() << Ha.toHex() << endl;
    // qDebug() << "Date & Time : "     << GPSdate.toString() << "," << GPStime.toString() << endl;
    // qDebug() << "Position (Lat,Lon,Alt): "   << latitud << "," << longitud << "," << height << endl;
    // qDebug() << "Satellites (Visible, Tracked): "  << QString::number(nvs, 10) << "," << QString::number(nts, 10)<< endl ;
    // qDebug() << "ID" << ID << endl;
}

void Dialog::Hn_Data(QByteArray Hn)
{
    // 78 Bytes
    quint8 SAT_ID;
    // Start @ @ H n Data:   0,1,2,3
    pulse_status=Hn.at(4); // 0 = off; 1 = on
    PPS_sync=Hn.at(5);
    TimeRAIM_solution=Hn.at(6);
    TimeRAIM_status=Hn.at(7);
    TimeRAIM_svid=(Hn.at(8) << 24 & 0xFF000000) | (Hn.at(9) << 16 & 0x00FF0000) | (Hn.at(10) << 8 & 0x0000FF00)| (Hn.at(11) & 0x000000FF);
    time_solution=(Hn.at(12)<<8 & 0xFF00) | (Hn.at(13) & 0x00FF);
    last_negative_sawtooth=negative_sawtooth;  // Guardo el anterior
    negative_sawtooth=Hn.at(14);               // Actualizo el sawtooth

    // Satellite data are from byte 15
    for(int i=0 ; i < 12 ; i++)
    {
        SAT_ID=Hn.at(15+i*5); //15
        if (SAT_ID < 33){
            SVID[SAT_ID].fractional_GPS_time=(Hn.at(16+i*5) << 24 & 0xFF000000) | (Hn.at(17+i*5) << 16 & 0x00FF0000) | (Hn.at(18+i*5) << 8 & 0x0000FF00)| (Hn.at(19+i*5) & 0x000000FF);
     //   qDebug() << SAT_ID << SVID[SAT_ID].mode << SVID[SAT_ID].fractional_GPS_time;
        }
    }

//    //Print Debugs
//        if(!pulse_status){
//            qDebug() << "Pulse Status: OFF";
//        }
//        else{
//            qDebug() << "Pulse Status: ON";
//        }
//        if(!PPS_sync){
//            qDebug() << "Pulse Sync: UTC";
//        }
//        else{
//            qDebug() << "Pulse Sync: GPS";
//        }
//        qDebug() << "T-RAIM solution:" << QString::number(TimeRAIM_solution);
//        qDebug() << "T-RAIM status:" << QString::number(TimeRAIM_status);
//        qDebug() << "T-RAIM SVID:" << QString::number(TimeRAIM_svid,16);
//        qDebug() << "1 sigma accuracy:" << QString::number(time_solution);
//        qDebug() << "Negative Sawtooth" << QString::number(negative_sawtooth) << endl;
}

void Dialog::Bb_Data(QByteArray Bb)
{
    // 92 Bytes
    quint8 SAT_ID;
    // quint8  number_sats;
    // Start @ @ B b Data:   0,1,2,3
    // number_sats = Bb.at(4);
    // Satellite data are from byte 5 to 89
    for(int i=0 ; i < 12 ; i++)
    {
         SAT_ID=Bb.at(5+i*7); // 63
         if (SAT_ID < 33){
         SVID[SAT_ID].Doppler=(Bb.at(6+i*7)<<8 & 0xFF00) | (Bb.at(7+i*7) & 0x00FF);
         SVID[SAT_ID].elevation=Bb.at(8+i*7);
         SVID[SAT_ID].azimuth=(Bb.at(9+i*7)<<8 & 0xFF00) | (Bb.at(10+i*7) & 0x00FF);
         SVID[SAT_ID].sat_health=Bb.at(11+i*7);
         }
    }
}

void Dialog::TIC_parse_data()
{
    //count=TIC_data_value;
    samples+=1;    //update samples
     csum=csum+count;
     mean=csum/samples;


}

//Send GPS commands
void Dialog::sendcommand(unsigned char *gpscommand, quint16 size, quint16 response)
{   //
    serialData.clear();         // clear received data buffer
    response_lenght=response;   // expected response length

    for (quint16 i=0; i < size; i++)
    {
        GPS->putChar(gpscommand[i]);

    }

}

void Dialog::SendPosition()
{
    // open the position file and send it to the receiver'
            QFile file(config_path + "/binary.pos");

            if (!file.open(QFile::ReadOnly))
            {
                qDebug() << "File not open" << endl;
            }
            else
            {
                QByteArray Position = file.readAll();
                qDebug() << "Position" << Position << endl;
                qDebug() << "Position size" << Position.size() << endl;
                file.close();

                quint8 checksum = 0x32;          // calculate checksum
                GPS->putChar('@');
                GPS->putChar('@');
                GPS->putChar(0x41);
                GPS->putChar(0x73);

                serialData.clear();   // clear received data buffer
                response_lenght=20;   // expected response length
            for (quint16 i=0; i < Position.size(); i++)
            {
                GPS->putChar(Position.at(i) & 0x00FF);
                checksum = checksum ^ Position.at(i);
            }

            GPS->putChar(0x00);
            GPS->putChar(checksum);
    // qDebug() << "chk"<< QString::number(checksum,16) <<endl;
            GPS->putChar(0x0D);
            GPS->putChar(0x0A);
            }


}

void Dialog::SendPositionHold()
{
    //  @@Gd  Set Position Hold
    static unsigned char Gd[]={0x40, 0x40, 0x47, 0x64, 0x01, 0x22, 0x0D, 0x0A};
    sendcommand(Gd,sizeof(Gd),8);

}

void Dialog::ReceiverStatus(quint16 rs)
{

   // qDebug() << "Status" << QString::number(rs,16) << endl;
    //Test Bits 13-15:
    switch((rs >> 13) & 0x0007)
    {
        case 0x00: status="Reserved";break;
        case 0x01: status="Reserved";break;
        case 0x02: status="Bad Geometry";break;
        case 0x03: status="Adquiring Satellites";break;
        case 0x04: status="Position Hold";break;
        case 0x05: status="Propagate Mode";break;
        case 0x06: status="2D Fix";break;
        case 0x07: status="3D Fix";break;
    }
    //Bits 11-12: Reserved

    //Test Bit10: Narrow band tracking mode
    if((rs&0x0400)){
        status=status+" Narrow Band";
    }
    //Test Bit9: Fast Acquisition Position
    if((rs&0x0200)){
        status=status+" Fast Acq";
    }
    //Test Bit8: Filter Reset To Raw GPS Solution
    if((rs&0x0100)){
        status=status+" Filter Reset";
    }

    //Test Bit7: Cold Start
    if((rs&0x0080)){
        status=status+" Cold Start";
    }
    //Test Bit6: Diferential Fix
    if((rs&0x0040)){
        status=status+" Differential Fix";
    }
    //Test Bit5: Position Lock
    if((rs&0x0020)){
        status=status+" Position Lock";
    }
    //Test Bit4: Autosurvey Mode
    if((rs&0x0010)){
        status="Survey Mode";
    }
    //Test Bit3: Insufficient Visible Satellites
    if((rs&0x0008)){
        status=status+" Insuf V. Sat";
    }

    //Test Bits 2-1:
    switch((rs >> 1) & 0x0003)
    {
        case 0x00: /*status=status+" Ant OK"*/;break;
        case 0x01: status="Ant OC";break;
        case 0x02: status="Ant UC";break;
        case 0x03: status="Ant NV";break;
    }
    //Test Bit0: Code Location
    if((rs&0x0001)){
       // status=status+" Internal";
    }else{
       // status=status+" External";
    }


}

void Dialog::ReadSettings()
{
    QString filename=config_path + "/device.dat";
    QFile file(filename);
    //if the file is not found, use the defaults defined here
    if (!QFile(filename).exists()) {


        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
         {
              qDebug() << "File device.dat not open" <<endl;
          }
         else
         {
            QTextStream outStream(&file);
            //default settings
            outStream << "001" << endl;               // Device number
            outStream << "SSTTF-TR" << endl;          // lab contact
            outStream << "CENAM" << endl;             // lab name
            outStream << "(1pps)" << endl;            // time scale designation
            outStream << "N" << endl;                 // FTP toggle (Y or N)
            outStream << "0" << endl;                 // INT delay (ns)
            outStream << "161.9" << endl;             // CAB delay (ns)
            outStream << "17.96" << endl;             // REF delay (ns)
            outStream << "10 Mhz" << endl;            // time base setting (only 10 MHz)
            outStream << "10" << endl;                // mask angle (0 to 89)
            file.flush();
            file.close();

         }
    }
    else
    {

    qDebug() << "File device.dat exist" <<endl;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
     {
          qDebug() << "File device.dat not open" <<endl;
      }
     else
     {

    QTextStream in(&file);
    for (quint8 i=0; i<10; i++)
    {
       settings[i]=in.readLine();
       qDebug() << QString::number(i+1) << settings[i] << endl;
    }
      file.close();
      int_dly=settings[5].toDouble();
      cbl_dly=settings[6].toDouble();
      ref_dly=settings[7].toDouble();
      qDebug() << "INT_DLY = " << QString::number(int_dly,'f',2)
               << "CBL_DLY = " << QString::number(cbl_dly,'f',2)
               << "REF_DLY = " << QString::number(ref_dly,'f',2)
               << endl;


    }
    }
}

void Dialog::SettoDefaults()
{
    //@@Cf Set to Defaults
    static unsigned char Cf[]={0x40, 0x40, 0x43, 0x66, 0x25, 0x0D, 0x0A};
    sendcommand(Cf,sizeof(Cf),7);

}

void Dialog::TimeMessage()
{
    // Combined Time message @@Gb

    serialData.clear();   // clear received data buffer
    response_lenght=17;   // expected response length

    QDateTime Local_datetime=QDateTime::currentDateTimeUtc();
    Local_date=Local_datetime.date();
    Local_time=Local_datetime.time();


    quint8 checksum = 0x25;          // precalculate checksum Gb

    GPS->putChar(0x40);
    GPS->putChar(0x40);
    GPS->putChar(0x47);
    GPS->putChar(0x62);
    unsigned char Gb[9];
    // Date
    Gb[0] =  Local_date.month();
    Gb[1] =  Local_date.day();
    Gb[2] =  Local_date.year() >> 8;
    Gb[3] =  Local_date.year();
    // Time
    Gb[4] =  Local_time.hour();
    Gb[5] =  Local_time.minute();
    Gb[6] =  Local_time.second();
    // GMT Offset
    Gb[7] =  0x00;
    Gb[8] =  0x00;
    Gb[9] =  0x00;


    for (quint16 i=0; i < 10; i++)
    {
        GPS->putChar(Gb[i]);
        checksum = checksum ^ Gb[i];
    }

    GPS->putChar(checksum);
    GPS->putChar(0x0D);
    GPS->putChar(0x0A);

}

void Dialog::ReceiverID()
{
    //@@Cf Set to Defaults
    static unsigned char Cj[]={0x40, 0x40, 0x43, 0x6A, 0x29, 0x0D, 0x0A};
    sendcommand(Cj,sizeof(Cj),588);
}

void Dialog::EnableTRAIM()
{

//    TIME RAIM SELECT MESSAGE
//    @@Ge
//    Query
//    0x40 0x40 0x47 0x65 0xFF 0xDD 0x0D 0x0A
//    Disable TRAIM
//    0x40 0x40 0x47 0x65 0x00 0x22 0x0D 0x0A
//    Enable TRAIM
//    0x40 0x40 0x47 0x65 0x01 0x23 0x0D 0x0A
    static unsigned char Ge[]={0x40, 0x40, 0x47, 0x65, 0x01, 0x23, 0x0D, 0x0A};
    sendcommand(Ge,sizeof(Ge),8);
}

void Dialog::StartPolling()
{
    //@@Hn Time RAIM Status Message
     static unsigned char Bb[]={0x40, 0x40, 0x42, 0x62, 0x01, 0x21, 0x0D, 0x0A};
     static unsigned char Ha[]={0x40, 0x40, 0x48, 0x61, 0x01, 0x28, 0x0D, 0x0A};
     static unsigned char Hn[]={0x40, 0x40, 0x48, 0x6E, 0x01, 0x27, 0x0D, 0x0A};
    //Send Triple Command
     sendcommand(Bb,sizeof(Bb),0);
     sendcommand(Ha,sizeof(Ha),0);
     sendcommand(Hn,sizeof(Hn),0);
}

void Dialog::StopPolling()
{
   //@@Hn Time RAIM Status Message
    static unsigned char Bb[]={0x40, 0x40, 0x42, 0x62, 0x00, 0x20, 0x0D, 0x0A};
    static unsigned char Ha[]={0x40, 0x40, 0x48, 0x61, 0x00, 0x29, 0x0D, 0x0A};
    static unsigned char Hn[]={0x40, 0x40, 0x48, 0x6E, 0x00, 0x26, 0x0D, 0x0A};
   //Send Triple Command
    sendcommand(Bb,sizeof(Bb),0);
    sendcommand(Ha,sizeof(Ha),0);
    sendcommand(Hn,sizeof(Hn),0);
}

void Dialog::PPSOneSat()
{
//    1PPS CONTROL MESSAGE
//    @@Gc
//    1PPS Disabled
//    0x40 0x40 0x47 0x63 0x00 0x24 0x0D 0x0A
//    1PPS Always
//    0x40 0x40 0x47 0x63 0x01 0x25 0x0D 0x0A
//    1PPS Active at least one satellite
//    0x40 0x40 0x47 0x63 0x02 0x26 0x0D 0x0A
//    1PPS Active when T-RAIM conditions are met
//    0x40 0x40 0x47 0x63 0x03 0x27 0x0D 0x0A

    static unsigned char Gc[]={0x40, 0x40, 0x47, 0x63, 0x02, 0x26, 0x0D, 0x0A};
    sendcommand(Gc,sizeof(Gc),8);

}

void Dialog::SurveyPosition()
{
    static unsigned char Gd[]={0x40, 0x40, 0x47, 0x64, 0x03, 0x20, 0x0D, 0x0A};   //@@Gd 0x03 autosurvey
    sendcommand(Gd,sizeof(Gd),8);
}

void Dialog::UpdateUTC()
{
    ui->lblDateTime->setText(GPSdate.toString(Qt::ISODate) + "/" + GPStime.toString(Qt::ISODate) );
    SaveData();
}

quint32 Dialog::JulianDate(QDate MJD)
{
    // This routine calculate the Modified Julian Data
    // input:    QDate
    // output:   Quint32 MJD
    return MJD.toJulianDay()-2400001;
}

void Dialog::LLAtoECEF(double lat, double lon, double h)
{
//    The basic formulas for converting from latitude, longitude, altitude
//    (above reference ellipsoid) to Cartesian ECEF are given in the
//    Astronomical Almanac in Appendix K.  They depend upon the following
//    quantities:

//         a   the equatorial earth radius
//         f   the "flattening" parameter ( = (a-b)/a ,the ratio of the
//             difference between the equatorial and polar radio to a;
//             this is a measure of how "elliptical" a polar cross-section
//             is).

//    The eccentricity e of the figure of the earth is found from

//        e^2 = 2f - f^2 ,  or  e = sqrt(2f-f^2) .

//    For WGS84,

//             a   = 6378137 meters
//           (1/f) = 298.257224

//    (the reciprocal of f is usually given instead of f itself, because the
//    reciprocal is so close to an integer)

//    Given latitude (geodetic latitude, not geocentric latitude!), compute

//                                      1
//            C =  ---------------------------------------------------
//                 sqrt( cos^2(latitude) + (1-f)^2 * sin^2(latitude) )

//    and
//            S = (1-f)^2 * C .

//    Then a point with (geodetic) latitude "lat," longitude "lon," and
//    altitude h above the reference ellipsoid has ECEF coordinates

//           x = (aC+h)cos(lat)cos(lon)
//           y = (aC+h)cos(lat)sin(lon)
//           z = (aS+h)sin(lat)



    double cosLat = qCos(lat * M_PI / 180.0);
    double cosLon = qCos(lon * M_PI / 180.0);
    double sinLat = qSin(lat * M_PI / 180.0);
    double sinLon = qSin(lon * M_PI / 180.0);

    double a = 6378137;
    double f= 1.0/298.257224;
    double C=1.0/(qSqrt((cosLat*cosLat)+((1.0-f)*(1.0-f)*sinLat*sinLat)));
    double S=(1.0-f)*(1.0-f)*C;


    ECEF_X = (a * C + h) * cosLat * cosLon;
    ECEF_Y = (a * C + h) * cosLat * sinLon;
    ECEF_Z = (a * S + h) * sinLat;


    qDebug() << "lat" << QString::number(lat,'f',6) << "°" <<
                "lon" << QString::number(lon,'f',6) << "°" <<
                "alt" << QString::number(h,'f',2) << "m" <<
                endl;


    qDebug() << "x" << QString::number(ECEF_X,'f',2) << "m" <<
                "y" << QString::number(ECEF_Y,'f',2) << "m" <<
                "z" << QString::number(ECEF_Z,'f',2) << "m" <<
                endl;
//    http://www.oc.nps.edu/oc2902w/coord/llhxyz.htm
}

void Dialog::SaveData()
{
    // this flag is false unless a new file is started later in the routine
    //bool newfile=false;
    QString oldname=namefile;
    QString extension=".csv";       // filename extension

    // define file name
    namefile= QString::number(JulianDate(GPSdate))  + extension;

    if (namefile != oldname)
    {
        //newfile=true;

        QString filename= data_path + "/" + namefile;
        QFile file(filename);
        //if the file is not found, use the defaults defined here
        if (!QFile(filename).exists()) {
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
                  qDebug() << "no data file" <<endl;
              }
             else{
                LLAtoECEF(latitud,longitud,altitud);
                QTextStream outStream(&file);
                //default settings
                outStream << "#LAB = SSTTF-TR" << endl;                 // Device number
                outStream << "ECEF" << endl;
                outStream << "#X = " << QString::number(ECEF_X,'f',2) << " m (GPS)" << endl;            // X
                outStream << "#Y = " << QString::number(ECEF_Y,'f',2) << " m (GPS)" << endl;            // Y
                outStream << "#Z = " << QString::number(ECEF_Z,'f',2) << " m (GPS)" << endl;            // Z
                outStream << "#FRAME = ITRF2014" << endl;            //
                outStream << "#COMMENTS = PROTOTYPE" << endl;            //
                outStream << "#INT DLY = " << QString::number(int_dly,'f',2) << " ns" << endl;
                outStream << "#CAB DLY = " << QString::number(cbl_dly,'f',2) << " ns" << endl;
                outStream << "#REF DLY = " << QString::number(ref_dly,'f',2) << " ns" << endl;
                outStream << "#REF = UTC(TEST)" << endl;            //
                outStream <<  endl;            //
                outStream << "TIME,DIFF,SAWTOOTH" << endl;            //
                file.flush();
                file.close();
             }
        }


    }

    // Save one minute averages
  //  if (GPStime.second()==59){

       // qDebug() << "TEST 0K" << endl;
        QString filename= data_path + "/" + namefile;
        QFile file(filename);
        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        QTextStream outStream(&file);
        outStream << GPStime.toString("hhmmss")
                  << ","
                  << QString::number(count,'f',2)
                  << ","
                  << QString::number(last_negative_sawtooth)
                  << endl;
        file.flush();
        file.close();
  // } // end one minute averages


}

void Dialog::on_btnSurvey_clicked()
{
   SurveyPosition();
}

void Dialog::on_btnReset_clicked()
{
    qDebug() << "Send Reset" << endl;
    SettoDefaults();
}

void Dialog::on_btnSetPHold_clicked()
{
SendPositionHold();
}

void Dialog::on_btnEnableTRAIM_clicked()
{
    EnableTRAIM();
}

void Dialog::on_btnEnablePPS_clicked()
{
    PPSOneSat();
}

void Dialog::SetConstellationMode()
{
    // @@Sc Set Constellation Mode
    // Query
    // 0x40 0x40 0x53 0x63 0x00 0x30 0x0D 0x0A
    // Set Only GPS
    // 0x40 0x40 0x53 0x63 0x01 0x31 0x0D 0x0A
    static unsigned char Sc[]={0x40, 0x40, 0x53, 0x63, 0x01, 0x31, 0x0D, 0x0A};
    sendcommand(Sc,sizeof(Sc),8);
}

void Dialog::SetDateTimePPSAlignment()
{
   // @@St Set Date/Time/PPS Alignment
   // This command sets or queries the time & 1PPS alignment source.
   // Query
   // 0x40 0x40 0x53 0x74 0x00 0x27 0x0D 0x0A
   // Set to UTC(USNO) with leap seconds
   // 0x40 0x40 0x53 0x74 0x02 0x25 0x0D 0x0A
    static unsigned char St[]={0x40, 0x40, 0x53, 0x74, 0x02, 0x25, 0x0D, 0x0A};
    sendcommand(St,sizeof(St),8);
}

void Dialog::SavePosition()
{
    //Position file creation
    QFile file(config_path + "/binary.pos");

    if (!file.open(QFile::WriteOnly))
    {
        qDebug() << "File not open" <<endl;
    }
    else
    {
        file.write(rawpos);     // correct
        file.flush();
        file.close();
    }
}

/*

// Extendend SSR-M8T
GNSS Receiver Type
@@Sx
0x40 0x40 0x53 0x78 0x2B 0x0D 0x0A
01 00000001 - GPS
02 00000010 - GLONASS
04 00000100 - Galileo
08 00001000 - BeiDou
10 00010000 - QZSS
20 00100000 - other 1
40 01000000 - other 2
80 10000000 - SBAS



@@St Set Date/Time/PPS Alignment
This command sets or queries the time & 1PPS alignment source.
Query
0x40 0x40 0x53 0x74 0x00 0x27 0x0D 0x0A
Set to UTC(USNO) with leap seconds
0x40 0x40 0x53 0x74 0x02 0x25 0x0D 0x0A


@@Sa Report Satellite Tracking Information
This command queries detailed satellite tracking information (up to 24 satellites)
@@SaxC<CR><LF>
Poll
0x40 0x40 0x53 0x61 0x00 0x32 0x0D 0x0A
One Second
0x40 0x40 0x53 0x61 0x01 0x33 0x0D 0x0A


@@Sp Report Current Number of Leap Seconds
This command queries the current number of leap seconds
Query Current Number of Leap Seconds
@@SpC<CR><LF>
0x40 0x40 0x53 0x70 0x23 0x0D 0x0A


@@So Configure @@Ha/Hn/Bb Message
(Future enhancement - not implemented at this time.)
This command configures which satellites the @@Ha, @@Hn, and @@Bb
messages report.
Command to configure @@Ha/Hn/Bb Message
@@SoxC<CR><LF>
0x40 0x40 0x53 0x6F 0xFF 0xC3 0x0D 0x0A



@@Sm Save configuration to non-volatile memory
Command to save all settings to non-volatile memory
@@SmbC<CR><LF>
b= 0x01 or 0xA5 to save all data (all else = nothing saved)
0x40 0x40 0x53 0x6D 0x01 0x3F 0x0D 0x0A
*/

void Dialog::on_btnConfig_clicked()
{
    SetConstellationMode();
}

void Dialog::on_btnStartPoll_clicked()
{
    StartPolling();
}

void Dialog::on_btnStopPoll_clicked()
{
    StopPolling();
}

void Dialog::on_btnSavePosition_clicked()
{
    SavePosition();
}


