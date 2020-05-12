#include "DA7212.h"
#include <cmath>
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"
#include "mbed.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "uLCD_4DGL.h"

DA7212 audio;

uLCD_4DGL uLCD(D1, D0, D2);
InterruptIn button(SW2);
DigitalOut led(LED2);
DigitalOut led2(LED1);
DigitalOut led3(LED3);
Serial pc(USBTX, USBRX);
Thread t(osPriorityNormal, 120 * 1024 /*120K stack size*/);

void playNote(int freq)
{
  int16_t waveform[kAudioTxBufferSize];
  for(int i = 0; i < kAudioTxBufferSize; i++)
  {
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  audio.spk.play(waveform, kAudioTxBufferSize);
}


int PredictGesture(float* output) {
  // How many times the most recent gesture has been matched in a row
  static int continuous_count = 0;
  // The result of the last prediction
  static int last_predict = -1;

  // Find whichever output has a probability > 0.8 (they sum to 1)
  int this_predict = -1;
  for (int i = 0; i < label_num; i++) {
    if (output[i] > 0.8) this_predict = i;
  }

  // No gesture was detected above the threshold
  if (this_predict == -1) {
    continuous_count = 0;
    last_predict = label_num;
    return label_num;
  }

  if (last_predict == this_predict) {
    continuous_count += 1;
  } else {
    continuous_count = 0;
  }
  last_predict = this_predict;

  // If we haven't yet had enough consecutive matches for this gesture,
  // report a negative result
  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
    return label_num;
  }
  // Otherwise, we've seen a positive result, so clear all our variables
  // and report it
  continuous_count = 0;
  last_predict = -1;

  return this_predict;
}

int LittleStar[42];
int LSLength[42];

int TwoTigers[42];
int TTLength[42];

int SunnyDay[42];
int SDLength[42];

int songLength[3]  {42, 32, 17};
int gesture_index;

int mode_sel;
int func, song;
char* mode_list[4] = {"forward", "backward", "song select", "Taiko mode"};
char* song_list[3] = {"Little Star", "Two Tigers", "Sunny Day"};
int taiko[17] = {
  1,2,2,1,1,2,1,
  2,2,1,1,1,1,
  1,2,1,1
};




void ISR1(){
    led = !led;
    if(mode_sel == 0){
      mode_sel = 1; //mode select
      gesture_index = -1;
    }
    else if(mode_sel == 1 && func == 2){
      mode_sel = 2; // select song
      gesture_index = -1;
      led = 0;
      led3 = 0;
    } 
    else if(mode_sel == 1 && func == 3){
      mode_sel = 3; //Taiko mode
      led = 0;
      led3 = 0;
    }
    else{ //exit from forward/backward
      mode_sel = 0;
      led3 = 1;
      if(func == 0){ //exit from forward/backward
        song = (song+1) % 3;
      }
      else if(func == 1){
        song = (song+2) % 3;
      }
    }
}

void loadSignal(void)
{
  char serialInBuffer[5];
  led2 = 0;
  int i = 0;
  int serialCount = 0;
  audio.spk.pause();
  while(i < 91){ // load frequency
    if(pc.readable())
    {
      serialInBuffer[serialCount] = pc.getc();
      //pc.printf("%c\r\n", serialInBuffer[serialCount]);
      serialCount++;
      if(serialCount == 3)
      {
        serialInBuffer[serialCount] = '\0';
        if(i < 42) LittleStar[i] = (int) atof(serialInBuffer);
        else if(41 < i && i < 74) TwoTigers[i-42] = (int) atof(serialInBuffer);
        else SunnyDay[i-74] = (int) atof(serialInBuffer);
        //pc.printf("%d\r\n", LittleStar[i]);
        serialCount = 0;
        i++;
      }
    }
  }
  i = 0;
  while(i < 91){ // load note length
    if(pc.readable())
    {
      serialInBuffer[serialCount] = pc.getc();
      //pc.printf("%c\r\n", serialInBuffer[serialCount]);
      serialCount++;
      if(serialCount == 1)
      {
        serialInBuffer[serialCount] = '\0';
        if(i < 42) LSLength[i] = (int) atof(serialInBuffer);
        else if(41 < i && i < 74) TTLength[i-42] = (int) atof(serialInBuffer);
        else SDLength[i-74] = (int) atof(serialInBuffer);
        //pc.printf("%d\r\n", LittleStar[i]);
        serialCount = 0;
        i++;
      }
    }
  }
  for(int j = 0; j < 42; j++){
    pc.printf("%d ", LittleStar[j]);
  }
  led2 = 1;
}

void playMusic(int songName[42], int notelength[42], int songlength, float scale);
void main_model();

int main(int argc, char* argv[]) {
  led = 1;
  led3 = 1;
  mode_sel = 0;
  func = 0;
  song = 0;
  loadSignal();
  button.rise(&ISR1);
  t.start(main_model);

}


void playMusic(int songName[42], int notelength[42], int songlength, float scale){
  for(int i = 0; i < songlength && mode_sel == 0; i++)
  {
    int length = notelength[i];
    while(length--)
    {
      // the loop below will play the note for the duration of 1s
      for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize && mode_sel == 0; ++j)
      {
        playNote(songName[i]);
      }
      
      if(length < 1 && mode_sel == 0){
        ThisThread::sleep_for(scale*notelength[i]);
        audio.spk.pause();
        wait_us(10000);
      }
    }
  }
  audio.spk.pause();
}

void main_model(){
  // Create an area of memory to use for input, output, and intermediate arrays.
  // The size of this will depend on the model you're using, and may need to be
  // determined by experimentation.
  constexpr int kTensorArenaSize = 60 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];

  // Whether we should clear the buffer next time we fetch data
  bool should_clear_buffer = false;
  bool got_data = false;

  // The gesture index of the prediction


  // Set up logging.
  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return -1;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  static tflite::MicroOpResolver<6> micro_op_resolver;
  micro_op_resolver.AddBuiltin(
      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
                               tflite::ops::micro::Register_MAX_POOL_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                               tflite::ops::micro::Register_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
                               tflite::ops::micro::Register_FULLY_CONNECTED());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                               tflite::ops::micro::Register_SOFTMAX());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                               tflite::ops::micro::Register_RESHAPE(), 1);

  // Build an interpreter to run the model with
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  tflite::MicroInterpreter* interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors
  interpreter->AllocateTensors();

  // Obtain pointer to the model's input tensor
  TfLiteTensor* model_input = interpreter->input(0);
  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
      (model_input->dims->data[1] != config.seq_length) ||
      (model_input->dims->data[2] != kChannelNumber) ||
      (model_input->type != kTfLiteFloat32)) {
    error_reporter->Report("Bad input tensor parameters in model");
    return -1;
  }

  int input_length = model_input->bytes / sizeof(float);

  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
  if (setup_status != kTfLiteOk) {
    error_reporter->Report("Set up failed\n");
    return -1;
  }

  error_reporter->Report("Set up successful...\n");

  int temp;
  while (true) {
    //pc.printf("%d%d ", func, song);
    // Attempt to read new data from the accelerometer
    got_data = ReadAccelerometer(error_reporter, model_input->data.f,
                                 input_length, should_clear_buffer);

    // If there was no new data,
    // don't try to clear the buffer again and wait until next time
    if (!got_data) {
      should_clear_buffer = false;
      continue;
    }

    // Run inference, and report any error
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      error_reporter->Report("Invoke failed on index: %d\n", begin_index);
      continue;
    }

    // Analyze the results to obtain a prediction
    gesture_index = PredictGesture(interpreter->output(0)->data.f);

    // Clear the buffer next time we read data
    should_clear_buffer = gesture_index < label_num;
    if(gesture_index < label_num){
      //error_reporter->Report(config.output_message[gesture_index]);
    }
    while(mode_sel == 0){
        temp = song;
        uLCD.cls();
        uLCD.printf("\nPlaying: %s\n", song_list[song]);
        if(song == 0) playMusic(LittleStar, LSLength, songLength[song], 0.25);
        else if(song == 1) playMusic(TwoTigers, TTLength, songLength[song], 0.125);
        else if(song == 2) playMusic(SunnyDay, SDLength, songLength[2], 0.125); 
        wait_us(1000000);
        song = (song+1)%3;
    }
    if(mode_sel == 1){ // mode select
      if(song != temp) song = temp;
      audio.spk.pause();
      //pc.printf("%d", func);
      if(gesture_index < label_num){
        uLCD.cls();
        //error_reporter->Report(config.output_message[gesture_index]);
        if(gesture_index == 0){
          func = (func+1)%4;
        }
        else if(gesture_index == 1){
          func = (func+3)%4;
        }
        for(int i = 0; i < 4; i++){
          if(i == func) uLCD.color(RED);
          uLCD.printf("\n%s\n", mode_list[i]);
          if(i == func) uLCD.color(GREEN);
        }

      }
    }
    else if(mode_sel == 2){ // song select
      audio.spk.pause();
      if(gesture_index < label_num){
        uLCD.cls();
        //error_reporter->Report(config.output_message[gesture_index]);
        if(gesture_index == 0){
          song = (song+1)%3;
        }
        else{
          song = (song+2)%3;
        }
        for(int i = 0; i < 3; i++){
          if(i ==song) uLCD.color(RED);
          uLCD.printf("\n%s\n", song_list[i]);
          if(i == song) uLCD.color(GREEN);
        }
      }
    }
    else if(mode_sel == 3){
      float score = 0;
      uLCD.cls();
      for(int i = 3; i > 0; i-- ){
        uLCD.printf("\n%d\n", i);
        wait_us(1000000);
        uLCD.cls();
      }
      for(int j = 0; j < songLength[2]; j++){
        uLCD.printf(" %d ", taiko[j]);
      }
      for(int i = 0; i < songLength[2]; i++) // Little Star as Taiko mode
      {

        int length = SDLength[i];
        accmeter();
        if(d[2] > 0.9 && taiko[i] == 1) score++;
        else if(d[0] >0.9 && taiko[i] == 2) score++;
        else score = score;
        while(length--)
        {
          // the loop below will play the note for the duration of 1s
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j)
          {
            playNote(SunnyDay[i]);
          }
      
          if(length < 1){
            ThisThread::sleep_for(0.125*SDLength[i]);
            audio.spk.pause();
            wait_us(10000);
          }
        }
      }
      audio.spk.pause();
      uLCD.cls();
      uLCD.printf("\nYour score is: %.1f\n", score/17*100);
      wait_us(1000000);
      uLCD.cls();
      mode_sel = 1;
      led3 = 1;
      for(int i = 0; i < 4; i++){
        if(i == func) uLCD.color(RED);
        uLCD.printf("\n%s\n", mode_list[i]);
        if(i == func) uLCD.color(GREEN);
      }
    }
  }
}