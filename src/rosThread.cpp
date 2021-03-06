#include <tf/tf.h>


#include "rosThread.h"
#include <navigationISL/networkInfo.h>
#include <QDebug>
#include <qjson/parser.h>
#include <QDir>
#include <QFile>


RosThread::RosThread()
{
    shutdown = false;

}



void RosThread::work(){



    QString path = QDir::homePath();
    path.append("/fuerte_workspace/sandbox/configISL.json");

//    QString path;
//    path.append("../initialPoses.txt");
    // Read initial poses otherwise quit!!
    if(!readInitialPoses(path))
    {
        qDebug()<<"Initial poses file could not be read!! ";
        qDebug()<<"Quitting...";

        ros::shutdown();

        emit this->rosFinished();

        return;

    }


    srand(time(0));

    for(int i = 1 ; i<=numOfRobots; i++)
    {
        dataReceived[i] = true;
        for(int j = 1 ; j<=numOfRobots; j++)
        {
            adjM[i][j] = 0;
        }
       // bin[i][1] = rand()%300;
       // bin[i][2] = rand()%300;

      //  qDebug()<<bin[i][1]<<" "<<bin[i][2];

    }


    if(!ros::ok()){

        emit this->rosFinished();

        return;
    }

    emit rosStarted();

    ros::Rate loop(10);

    communicationManagerSubscriber = n.subscribe("communicationISL/coordinatorUpdate",5,&RosThread::handleCoordinatorUpdate,this);
    networkinfoPublisher = n.advertise<navigationISL::networkInfo>("coordinatorISL/networkInfo",1,true);

    while(ros::ok())
    {
        ros::spinOnce();
        loop.sleep();

        int activeRobots = 0;
        for(int i = 1; i <=numOfRobots; i++)
        {
            if (dataReceived[i]==true)
            {
                activeRobots = activeRobots + 1;
            }
        }

        // If all the data is received from the robots
        //if(dataReceived[1] && dataReceived[2] && dataReceived[3])
        if(activeRobots == numOfRobots)
        {

            // Play Game
            coordinator.playGame(adjM,adjM,bin);

            // Prepare the output string
            QString networkString;

            // For all the rows of the matrix
            for(int i = 1; i <=numOfRobots; i++)
            {

                // If the robot is active
                if(dataReceived[i])
                {
                    int neighborCount = 0;

                    // Search the entire row
                    for(int j = 1; j <=numOfRobots; j++)
                    {
                        // If a neighborhood is present, fill the message
                        if(adjM[i][j] == 1)
                        {
                            neighborCount++;

                            qDebug()<<"adjm ok "<<i<<" "<<j;

                            QString str = "IRobot";

                            str.append(QString::number(j));

                            if(networkString.size() > 0 && networkString.at(networkString.size()-1) != ',')
                                networkString.append(";");


                            networkString.append(str);


                        }
                    }

                    dataReceived[i] = false;

                    if(neighborCount > 0)

                        networkString.append(",");

                    else
                    {
                        networkString.append("0");
                        networkString.append(",");
                    }
                }
            }

            networkString.truncate(networkString.size()-1);

            navigationISL::networkInfo info;

            info.network = networkString.toStdString();

            networkinfoPublisher.publish(info);
        }


    }

    qDebug()<<"I am quitting";

    ros::shutdown();

    emit rosFinished();


}
void RosThread::shutdownROS()
{
    ros::shutdown();
    // shutdown = true;


}
void RosThread::handleCoordinatorUpdate(navigationISL::neighborInfo info)
{
    navigationISL::neighborInfo inf = info;

    QString str = QString::fromStdString(inf.name);

    str.remove("IRobot");

    int id = str.toInt();

    bin[id][1] = inf.posX;

    bin[id][2] = inf.posY;

    dataReceived[id] = true;

}
bool RosThread::readInitialPoses(QString filename)
{
/*
    QFile file(filepath);

    if(!file.open(QFile::ReadOnly))
    {

        return false;
    }

    QTextStream stream(&file);
    int count  = 1;
    while(!stream.atEnd())
    {
        QString str = stream.readLine();

        QStringList poses = str.split(",");

        bin[count][1] = poses.at(0).toDouble();
        bin[count][2] = poses.at(1).toDouble();

        qDebug()<<"Pose "<<count<<" "<<bin[count][1]<<" "<<bin[count][2];
        count++;

    }

    file.close();

    return true;
*/

    QFile file(filename);

    if(!file.exists()) return false;

    if(!file.open(QFile::ReadOnly)) return false;

    QJson::Parser parser;

    bool ok;

    QVariantMap result = parser.parse(&file,&ok).toMap();

    if(!ok){

        file.close();
        qDebug()<<"Fatal error - reading initial poses";

        return false;
    }
    else
    {
        int robotID = result["robotID"].toInt();

        qDebug()<<result["robotID"].toString();

        double initialX = result["initialX"].toDouble();

        double initialY = result["initialY"].toDouble();


        // initial pozisyonlar bin'e ataniyor

        bin[robotID][1] = initialX;
        bin[robotID][2] = initialY;

        qDebug() << "Pose "<< robotID << " " << bin[robotID][1] << " " << bin[robotID][2];

        QVariantMap nestedMap = result["Robots"].toMap();

        foreach (QVariant plugin, nestedMap["Robot"].toList()) {

            QString rNameStr = plugin.toMap()["name"].toString();

            rNameStr.remove("IRobot");

            int rID = rNameStr.toInt();

            bin[rID][1] = plugin.toMap()["initialX"].toDouble();
            bin[rID][2] = plugin.toMap()["initialY"].toDouble();


            qDebug() << "Pose " << rID << " " << bin[rID][1] << " " << bin[rID][2];
        }



    }
    file.close();
    return true;

}
