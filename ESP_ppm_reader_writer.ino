#define PPM_FRAME_LENGTH 22500
#define PPM_PULSE_LENGTH 300
#define PPM_CHANNELS 6
#define DEFAULT_CHANNEL_VALUE 1500
#define OUTPUT_PIN 13
#define INPUT_PIN 34

uint16_t channelValue[PPM_CHANNELS] = {1500, 1500, 1500, 1500, 1500, 1500};
uint16_t channelValue_r[PPM_CHANNELS] = {0, 0, 0, 0, 0, 0};
uint64_t timerValue;

hw_timer_t *timer = NULL;
hw_timer_t *timer_r = NULL;

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

enum ppmState_e
{
    PPM_STATE_IDLE,
    PPM_STATE_PULSE,
    PPM_STATE_FILL,
    PPM_STATE_SYNC
};

enum ppmState_re
{
    PPM_STATE_IDLE_R,
    PPM_STATE_READ_R
};

void IRAM_ATTR ppmRead()
{
    static uint8_t ppmState_r = PPM_STATE_IDLE_R;
    static uint8_t ppmChannel_r = 0;
    int currentChannelValue;
   

    timerValue = timerReadMicros(timer_r);
    timerRestart(timer_r);

    portENTER_CRITICAL(&timerMux);

    if (ppmState_r == PPM_STATE_IDLE_R)
    {
      if (timerValue>2100)
      {
        ppmChannel_r = 0;
        ppmState_r = PPM_STATE_READ_R;
      }
    }

    if (ppmState_r == PPM_STATE_READ_R)
    {
        if (timerValue<2100 && ppmChannel_r<6)
        {
            channelValue_r[ppmChannel_r] = timerValue;
            ppmChannel_r++;
        }
        else if (timerValue>2100)
        { 
            ppmChannel_r = 0;
        }
    }
    portEXIT_CRITICAL(&timerMux);
}

void IRAM_ATTR onPpmTimer()
{

    static uint8_t ppmState = PPM_STATE_IDLE;
    static uint8_t ppmChannel = 0;
    static uint8_t ppmOutput = LOW;
    static int usedFrameLength = 0;
    int currentChannelValue;

    portENTER_CRITICAL(&timerMux);

    if (ppmState == PPM_STATE_IDLE)
    {
        ppmState = PPM_STATE_PULSE;
        ppmChannel = 0;
        usedFrameLength = 0;
        ppmOutput = LOW;
    }

    if (ppmState == PPM_STATE_PULSE)
    {
        ppmOutput = HIGH;
        usedFrameLength += PPM_PULSE_LENGTH;
        ppmState = PPM_STATE_FILL;

        timerAlarmWrite(timer, PPM_PULSE_LENGTH, true);
    }
    else if (ppmState == PPM_STATE_FILL)
    {
        ppmOutput = LOW;
        currentChannelValue = channelValue[ppmChannel];

        ppmChannel++;
        ppmState = PPM_STATE_PULSE;

        if (ppmChannel >= PPM_CHANNELS)
        {
            ppmChannel = 0;
            timerAlarmWrite(timer, PPM_FRAME_LENGTH - usedFrameLength, true);
            usedFrameLength = 0;
        }
        else
        {
            usedFrameLength += currentChannelValue - PPM_PULSE_LENGTH;
            timerAlarmWrite(timer, currentChannelValue - PPM_PULSE_LENGTH, true);
        }
    }
    portEXIT_CRITICAL(&timerMux);
    digitalWrite(OUTPUT_PIN, ppmOutput);
}

void setup()
{
    Serial.begin(115200);
    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(INPUT_PIN, INPUT);

    timer = timerBegin(0, 80, true);
    timer_r = timerBegin(1,80,true);

    timerAttachInterrupt(timer, &onPpmTimer, true);
    timerAlarmWrite(timer, 12000, true);
    timerAlarmEnable(timer);

    attachInterrupt(INPUT_PIN, &ppmRead, RISING);
    Serial.println("Hello");
}

void loop()
{
    /*
    Here you can modify the content of channelValue array and it will be automatically
    picked up by the code and outputted as PPM stream. For example:
    */
    channelValue[0] = channelValue_r[0];
    channelValue[1] = channelValue_r[1];
    channelValue[2] = channelValue_r[2];
    channelValue[3] = channelValue_r[3];
    //Serial.println(channelValue_r[0]);
   // Serial.println(timerValue);
}