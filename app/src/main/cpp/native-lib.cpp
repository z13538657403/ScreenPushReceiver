#include <stdio.h>
#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "jrtplib3/rtpsession.h"
#include "jrtplib3/rtpudpv4transmitter.h"
#include "jrtplib3/rtpipv4address.h"
#include "jrtplib3/rtpsessionparams.h"
#include "jrtplib3/rtppacket.h"
#include "jrtplib3/rtperrors.h"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"Native_Log",__VA_ARGS__)
#define H264 96
#define BUFFER_SIZE 1000 * 1024

using namespace std;
using namespace jrtplib;

static volatile int gIsThreadExit = 0;
JavaVM *gJavaVM;
jobject gJavaObj;
RTPSession session;
unsigned char m_buffer[BUFFER_SIZE];
unsigned char * m_pBuffer;
size_t m_current_size;

static void* get_rec_data(void *arg)
{
    JNIEnv *env;
    gJavaVM->AttachCurrentThread(&env , NULL);
    jclass javaClass = env->GetObjectClass(gJavaObj);
    if(javaClass == NULL)
    {
        LOGD("failed to find JavaClass");
        return 0;
    }

    jmethodID javaCallBack = env->GetMethodID(javaClass , "onFrame" , "([BI)Z");
    if(javaCallBack == NULL)
    {
        LOGD("failed to find method onFrame");
        return 0;
    }

    while(!gIsThreadExit)
    {
        session.Poll();
        session.BeginDataAccess();
        if (session.GotoFirstSourceWithData())
        {
            do
            {
                RTPPacket *pack;
                while((pack = session.GetNextPacket()) != NULL)
                {
                    if(pack->GetPayloadType() == H264)
                    {
                        //the last packet
                        if(pack->HasMarker())
                        {
                            LOGD("the recieve total =  %d" , m_current_size +  pack->GetPayloadLength());
                            memcpy(&m_buffer[m_current_size] , pack->GetPayloadData() , pack->GetPayloadLength());
                            memcpy(m_pBuffer , m_buffer , m_current_size +  pack->GetPayloadLength());

                            jbyteArray RtnArr = NULL;
                            RtnArr = env->NewByteArray(m_current_size +  pack->GetPayloadLength());
                            env->SetByteArrayRegion(RtnArr , 0 , m_current_size +  pack->GetPayloadLength() , (jbyte*)m_pBuffer);
                            env->CallBooleanMethod(gJavaObj , javaCallBack , RtnArr , m_current_size + pack->GetPayloadLength());

                            memset(m_buffer , 0 , BUFFER_SIZE);
                            memset(m_pBuffer , 0 , BUFFER_SIZE);
                            env->DeleteLocalRef(RtnArr);
                            m_current_size = 0;
                        }
                        else
                        {
                            //大的H264数据，分几个包发送过来
                            memcpy(&m_buffer[m_current_size] , pack->GetPayloadData() , pack->GetPayloadLength());
                            m_current_size += pack->GetPayloadLength();
                        }
                    }
                    else
                    {
                        // 非264数据包
                        LOGD("Not H264 Package.");
                    }

                    session.DeletePacket(pack);
                }
            }
            while (session.GotoNextSourceWithData());
        }
        session.EndDataAccess();
    }

    gJavaVM->DetachCurrentThread();
    LOGD("native thread exec loop leave.");
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_ckzn_jrtptest_MainActivity_initRtpLib(JNIEnv *env, jobject obj, jstring ip_, jint basePort, jint destPort)
{
    env->GetJavaVM(&gJavaVM);
    gJavaObj = env->NewGlobalRef(obj);

    RTPSessionParams sessionParams;
    RTPUDPv4TransmissionParams transmissionParams;
    int status = 0;

    sessionParams.SetOwnTimestampUnit(1.0/9000.0);
    sessionParams.SetAcceptOwnPackets(true);

    transmissionParams.SetPortbase((uint16_t)basePort);
    status = session.Create(sessionParams , &transmissionParams);
    LOGD("init error info 111= %s" , RTPGetErrorString(status).c_str());

    const char* ipStr = env->GetStringUTFChars(ip_ , 0);
    uint32_t destIp = ntohl(inet_addr(ipStr));
    RTPIPv4Address addr(destIp ,(uint16_t)destPort);
    status = session.AddDestination(addr);
    LOGD("init error info = %s" , RTPGetErrorString(status).c_str());

    memset(m_buffer,0,BUFFER_SIZE);
    m_pBuffer = new unsigned char[BUFFER_SIZE];
    m_current_size = 0;

    return 1;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_ckzn_jrtptest_MainActivity_receiveData(JNIEnv *env, jobject instance)
{
    gIsThreadExit = 0;
    pthread_t threadId;
    if(pthread_create(&threadId , NULL , get_rec_data , NULL) != 0)
    {
        LOGD("native thread start thread create failed");
        return 0;
    }
    LOGD("native thread create success");
    return 1;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_ckzn_jrtptest_MainActivity_deInitRtpLib(JNIEnv *env, jobject instance)
{
    gIsThreadExit = 1;
    session.Destroy();
    return 1;
}