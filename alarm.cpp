#include <filesystem>
#include <iostream>
#include <string>
#include <chrono>
#include <cstring>
#include <thread>
#include <mqueue.h>
#include <fstream>
#include <queue>
#include <random>

/*
TODO: A LOT of error checking needed, for now itll hope that the user will input the commands correctly
TODO: Mutex are for the weak
 */

#define CHRONOMETER_QUEUE  "/chronometer"
#define ALARM_QUEUE "/alarm"
#define POMODORO_QUEUE "/pomodoro"


struct parameters {
    bool Pomodoro = false;
    bool Repeat = false;
    bool Chronometer = false;
    bool Periodic = false;
    bool Alarm = false;
};

struct idDeletion {
    unsigned int id;
    int position;
};

struct Alarm {
    int time;
    unsigned int id;
    std::string name;
    std::string soundPath;
    bool repeat = false;
    bool periodic = false;
    bool days[7] = {false};
    bool handled = false;
    bool enabled = true;
    std::thread *thread;
};


typedef struct {
    char msg[10];
    unsigned int id;
} Message;



mq_attr attr = {
    .mq_flags = 0,
    .mq_maxmsg = 10,
    .mq_msgsize = sizeof(Message),
    .mq_curmsgs = 0
}; // Message queue attributes

typedef std::chrono::duration<float> fseconds;
typedef std::chrono::duration<float, std::ratio<60> > fminutes;
typedef std::chrono::duration<float, std::ratio<3600> > fhours;

void WriteAlarm(const Alarm &alarm);
int WaitAndCheck(std::deque <Alarm> &alarms, bool handled);
void PlaySound(std::string audiofile, bool ShowCommand = false);
void GetSoundFiles(std::string path);
void SendNotification(std::string title, std::string body);
void RemindMe(float time, char format, bool ShowTime = false);
void ChronometerStart(mq_attr attr, bool debug = false);
void ChronometerStop(mq_attr attr);
void StartAlarmService(void);
void NewAlarmExists(mq_attr attr);
void PomodoroStart(mq_attr attr, int pomodoroDuration = 2400, int restingDuration = 300);// 2400 300
void PomodoroStop(mq_attr attr);
void PomodoroPause(mq_attr attr);
void PomodoroResume(mq_attr attr);
//void testingtwo(mq_attr attr);
//void testing(mq_attr attr);
bool CheckNewAlarm(mq_attr attr);
bool isDay(Alarm alarm);
bool isFuture(Alarm alarm);
bool DoesFileExist(std::string fileName);
bool ReadAlarms(std::deque<Alarm> &alarms);
time_t itoTime (int time);
time_t EvilPointToTime (std::chrono::system_clock::time_point time, bool debug = false);
std::chrono::system_clock::time_point currentTime (std::chrono::system_clock::time_point time);
std::string daysToString(const Alarm &alarm);
Alarm stringToDays(std::string input, Alarm& alarm);
void BetterAlarmService(mq_attr attr);
void ActivateAlarm(Alarm &alarm, std::deque <unsigned int> &deletedIds);
void HandleAlarms(std::deque <Alarm> &alarms, std::deque<unsigned int> &deletedIds);
void HandleThreads(std::deque<Alarm>& alarms);
unsigned int randInt(int min = 0, int max = UINT16_MAX);
void toggleAlarm(mq_attr attr, unsigned int id);
void deleteAlarm(mq_attr attr, unsigned int id);
void clearAlarms(void);
void callNewAlarm(mq_attr attr);



namespace fs = std::filesystem;
namespace chrono = std::chrono;


int main(int argc, char *argv[]) {
    parameters Params;

    std::string path = fs::current_path();
    std::string SoundsPath = fs::current_path();
    SoundsPath += "/Sounds/";
    const std::string defaultSound = path  + "/Sounds/Alarm.mp3";


    Alarm alarm;
    Alarm test;

    std::deque <Alarm> alarms;
    //Goes through every argument, and takes the second character and runs code based on it
    for (int arg = 1; arg < argc; arg++) {
        switch (argv[arg][1]) {
            case 'a': {
                // format will be -a (if -c =>"customsoundpath") "alarmname" -(r || -R) -(u || p)
                Params.Alarm = true; // We are adding a new alarm
                arg++; //Go to the next arg
                std::cout << argv[arg] << std::endl;
                if (argv[arg][0] == '-' && argv[arg][1] == 'c') {
                    arg++;
                    alarm.soundPath = argv[arg];
                    std::cout << alarm.soundPath << std::endl;
                    arg++;
                }
                else
                    alarm.soundPath = defaultSound;
                 alarm.name = argv[arg];
                arg++; //Go to the next arg
                std::cout << argv[arg] << std::endl;
                switch (argv[arg][1]) {
                    // Set repeat for alarm
                    case 'r':
                        alarm.repeat = false;
                        Params.Repeat = false;
                    break;
                    case 'R':
                        alarm.repeat = true;
                        Params.Repeat = true;
                    break;
                    default:
                        std::cout << "error in repeat" << std::endl;
                    return -1;
                    break;
                }
                arg++; //Go to the next arg
                switch (argv[arg][1]) {
                    // Set Periodic or unique
                    case 'p':
                        alarm.periodic = true;
                        Params.Periodic = true;
                    arg++;
                    std::cout << "Days is set as: " << argv[arg] << std::endl;
                    //Go to the next arg, should be a string that looks something like this 0000000, with 0 being false and 1 being true
                    for (int idx = 0; idx < 7; idx++) {
                        // Set what days
                        argv[arg][idx] == '0' ? alarm.days[idx] = 1 : alarm.days[idx] = 0;
                        std::cout << "Days: " << argv[arg][idx] << std::endl;
                    }
                    break;
                    case 'u':
                        Params.Periodic = false;
                        alarm.periodic = false;
                    break;
                    default:
                        std::cout << "error2" << std::endl;
                    return -1;
                    break;
                }
                arg++;
                alarm.time = itoTime(std::stoi(argv[arg]));
                arg = 100; // stops taking more argument, yes i know this is probably a bad way to do it
                alarm.id = randInt(); //lets just hope i dont get 2 equal ids
                WriteAlarm(alarm);
                NewAlarmExists(attr);
            }break;
            case 'C':
                Params.Chronometer = true;
                ChronometerStart(attr);
            break;
            case 'c':
                ChronometerStop(attr);
            break;
            case 'R'://something like alarm -R -s
            {
                std::cout << "this is running" << std::endl;
                RemindMe(std::stof(argv[arg + 1]), argv[arg+2][1], true);
                PlaySound(defaultSound);
            }
            break;
            case 'S':
                //StartAlarmService();
                    BetterAlarmService(attr);
            break;
            case 'i':
                GetSoundFiles(SoundsPath);
                arg = 100;
                break;
            case 'P': //Will handle pomodoro mode, s=start, p=pause, r=resume, o=stop
                switch (argv[arg][2]) {
                    case 's':
                        PomodoroStart(attr);
                    break;
                    case 'r':
                        PomodoroResume(attr);
                    break;
                    case 'p':
                        PomodoroPause(attr);
                    break;
                    case 'o':
                        PomodoroStop(attr);
                    break;
                }
            break;
            case 'D':
                arg++;
                    deleteAlarm(attr, std::stoi(argv[arg]));
                std::cout<< std::endl << "testing";
                arg = 100;
            break;
            case 'f':
                callNewAlarm(attr);
            break;
        }
    }
    return 0;
}


/// <summary>
///     Prints all the files in path+/Sounds.
/// </summary>
/// <param name="path">Path where it'll read for all files</param>
void GetSoundFiles(std::string path) {
    for (const auto &entry: fs::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            std::cout << entry.path().filename().string() << std::endl;
        }
    }
}

/// <summary>
///     Runs paplay on the desired audiofile, which plays the sound
/// </summary>
/// <param name="audiofile">path to audiofile, including its name</param>
/// <param name="ShowCommand">Optional bool specifying whether the command that this function called is outputted or not.</param>
void PlaySound(std::string audiofile, bool ShowCommand) {
    std::string soundCommand;
    soundCommand = "\"" +"paplay " + audiofile + "\"";
    system(soundCommand.c_str());
    if (ShowCommand)
        std::cout << soundCommand << std::endl;
}


/// <summary>
///     Sends a notification with title, and body
/// </summary>
/// <param name="title">Title that will be displayed on the notification</param>
/// <param name="body">Body that will be displayed on the notification.</param>
void SendNotification(std::string title, std::string body) {
    std::string notifCommand;
    std::cout<< "Title: "<< title << "BODY: "<< body << std::endl;

    notifCommand = "notify-send \"" + title+ "\"" + " " +"\""+ body + "\"";
    std::cout << "Notification command: " << notifCommand << std::endl;
    system(notifCommand.c_str());
}


/// <summary>
///     Waits for "time" format(seconds, minutes, or hours).
/// </summary>
/// <param name="time">Time that will be converted to hours, minutes, or seconds depending on format</param>
/// <param name="format">Lets you choose between seconds, minutes, or hours</param>
void RemindMe(float time, char format, bool ShowTime) {
    switch (format) {
        case 's': {
            fseconds duration{time};
            std::this_thread::sleep_for(duration);
            if (ShowTime)
                std::cout << "Waited for " << duration.count() << " Seconds \n";
        }
        break;
        case 'm': {
            fminutes durationMinutes{time};
            std::this_thread::sleep_for(durationMinutes);
            if (ShowTime)
                std::cout << "Waited for " << durationMinutes.count() << " Minutes \n";
        } // I hate this language, the {} are needed to avoid getting an error on the switch statement because of variable declaration
        break;
        case 'h': {
            fhours durationHours{time};
            std::this_thread::sleep_for(durationHours);
            if (ShowTime)
                std::cout << "Waited for " << durationHours.count() << " Hours \n";
        }
        break;
        default:
            break;
    }
    SendNotification("RemindMe", "Reminding you");
}


/// <summary>
///     Starts the chronometer, and waits until ChronometerStop is called to get the duration between ChronometerStart and ChronometerStop, then sends a notification with the duration.
/// </summary>
/// <param name="attr">queue attributes</param>
/// <param name="debug">Optional bool specifying whether to output the received message and the time or not.</param>
void ChronometerStart(mq_attr attr, bool debug) {
    mq_unlink(CHRONOMETER_QUEUE);
    mqd_t mqd = mq_open(CHRONOMETER_QUEUE, O_CREAT | O_RDONLY, 0600, &attr);
    if (mqd == -1) {
        perror("Error opening queue");
        exit(EXIT_FAILURE);
    }

    Message receivedMessage;

    chrono::system_clock::time_point begin = chrono::system_clock::now(); //Starts the timer for the chronometer
    mq_receive(mqd, reinterpret_cast<char *>(&receivedMessage), sizeof(receivedMessage), nullptr);
    //waits until it recives a message
    chrono::system_clock::time_point end = chrono::system_clock::now(); // Gets a second time point to get the duration
    fseconds duration = chrono::duration_cast<fseconds>(end - begin);


    if (debug) {
        std::cout << "Message: " << receivedMessage.msg << std::endl;
        std::cout << "Time: " << duration << std::endl;
    }

    std::string body = "Time was: " + std::to_string(duration.count()) + " seconds"; //\"
    SendNotification("Chronometer", body);
}

/// <summary>
///     Stops the Chronometer
/// </summary>
/// <param name="attr">queue attributes</param>
void ChronometerStop(mq_attr attr) {
    mqd_t mqd = mq_open(CHRONOMETER_QUEUE, O_CREAT | O_WRONLY, &attr);
    Message message;
    strcpy(message.msg, "STOP");
    mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 1); // when i read about this, people just said "just use this", but what does reinterpret_cast<char *>(&message) do?
    //is it because i declared message as a struct with just a char[] inside? what would happen if the struct was something like struct {char message[10]; int a;}
    mq_close(mqd);
    mq_unlink(CHRONOMETER_QUEUE);
}

void StartAlarmService(void) {
    //should start and read for alarms, then wait until the closest alarm, then read and wait for the closest alarm and so and so
    while (true) {
        std::deque <Alarm> alarms;
        if (!ReadAlarms(alarms)) {
            std::cout<< std::endl << "Error starting Alarm Service: Alarm.txt doesnt exist";
            return;
        }
        bool newAlarm = false;
        while (!newAlarm) {
            int out = WaitAndCheck(alarms, false);
            switch (out) {
                case 0://alarm is ringing the alarm
                    break;
                case 1://theres a new alarm
                    newAlarm = true;
                    break;
                case 2://ring the alarm but ill do it on the function i guess
                    break;
                default:
                    break;
            }
        }
        //read if a new alarm was set
        //iterate through every alarm and then if one of them is equal to the current time then call alarm
    }
}

/// <summary>
///     Iterates through every alarm in "alarms", if an alarm is in the future, it will wait for the closest one (because alarms will always be sorted when this is called)
///     while it waits, it will check every "timeInterval" seconds if a new alarm has been added, if there's a new one, it stops there and returns 1, so that the function has
///     to be called again, and it'll have the new alarm
/// </summary>
/// <param name="alarms">Deque with every alarm sorted by time</param>
int WaitAndCheck(std::deque <Alarm> &alarms, bool handled) { // while doing this i realized i could also make a lot of processes that wait for a singular alarm but im too lazy now
    int currentTime;
    int timeInterval = 1; //how many seconds does it wait before checking if theres a new alarm
    //std::cout << "Test1" << std::endl;
    for (Alarm alarm : alarms) {
        currentTime = EvilPointToTime(chrono::system_clock::now());
        if (alarm.time >= currentTime) { // if the alarm is in the future
            std::cout << currentTime << "is current time" << std::endl;
            std::cout << alarm.time << " is next alarm" << std::endl;
            for (int time = currentTime; time < alarm.time; time+=timeInterval) { // wait until next alarm
                std::this_thread::sleep_for(std::chrono::seconds(timeInterval));
                //time+=timeInterval;
                if (CheckNewAlarm(attr)) { // TODO: i need to make something about this check, it only checks every 10 seconds. but what happens if the alarm time is between those 10 seconds?
                    return 1; // theres a new alarm
                }
            }
            PlaySound(alarm.soundPath);
            SendNotification(alarm.name, "Your alarm is ringing");
            //handle alarm
            return 2; // alarm moment
        }
        if (handled) //i think it would spam alarm the second the alarm is running if i dont make it wait
        return 0;
    }
    return 0;
}


/// <summary>
///     Returns true if a new alarm has been added to the alarm message queue
/// </summary>
/// <param name="attr">queue attributes</param>
bool CheckNewAlarm(mq_attr attr) {
    mq_unlink(ALARM_QUEUE);
    std::string alarmMessage = "NEW_ALARM";
    mqd_t mqd = mq_open(ALARM_QUEUE, O_CREAT | O_RDONLY | O_NONBLOCK, 0600, &attr);
    if (mqd == -1) {
        perror("Error opening queue");
        exit(EXIT_FAILURE);
    }
    Message receivedMessage;
    mq_receive(mqd, reinterpret_cast<char *>(&receivedMessage), sizeof(receivedMessage), nullptr);
    if (receivedMessage.msg == alarmMessage.c_str()) {
        mq_close(mqd);
        return true;
    }
    else {
        mq_close(mqd);
        return false;
    }
}

/// <summary>
///     First checks if there's already a new alarm that has been called, and if there isn't, it sends a message to the
///     alarm queue to say there's a new one. This was used for the first alarm service. BetterAlarmService() no longer
///     uses it
/// </summary>
/// <param name="attr">queue attributes</param>
void NewAlarmExists(mq_attr attr) {
    if (!CheckNewAlarm(attr)) {
        mqd_t mqd = mq_open(ALARM_QUEUE, O_CREAT | O_WRONLY, &attr);
        Message message;
        strcpy(message.msg, "NEW_ALARM");
        mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 1); // when i read about this, people just said "just use this", but what does reinterpret_cast<char *>(&message) do?
        //is it because i declared message as a struct with just a char[] inside? what would happen if the struct was something like struct {char message[10]; int a;}
        //Update: well now im using uint id and it works? idk what reinterpret cast does tbh
        mq_close(mqd);
        mq_unlink(ALARM_QUEUE);
    }
}




/// <summary>
///     Writes the alarm into Alarm.txt
/// </summary>
/// <param name="alarm">Alarm info</param>
void WriteAlarm(const Alarm &alarm) { // wip
    bool exists = DoesFileExist("Alarm.txt");
    std::fstream file;

    file.open("Alarm.txt", std::ios::out | std::ios::in | std::ios::app);
    file.seekp(1, std::ios::cur);
    /*
    if (file.get() == '#')
        file << "\n";
    */
    if (exists) {
        file << "\n";
    }
    file <<  alarm.name << "\n";
    file <<  alarm.id << "\n";
    file << alarm.soundPath << "\n";
    file << alarm.time << "\n";
    file << alarm.repeat << "\n";
    file << alarm.enabled << "\n";
    file << alarm.periodic << "\n";
    file << daysToString(alarm) << "\n";
    file << "###########################";
    file.close();


    return ;
}
/// <summary>
///     Reads all the alarms in alarm.txt, gets them in memory, and sorts them, returns false if the file doesnt exist
/// </summary>
/// <param name="alarms">A deque of alarms</param>
bool ReadAlarms(std::deque<Alarm> &alarms) {
    std::ifstream file;
    bool duplicate = false;
    if (!DoesFileExist("Alarm.txt"))
        return false;

    file.open("Alarm.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open Alarm.txt" << std::endl;
    }
    file.seekg(0, std::ios::beg);
    while (!file.eof()) {
        Alarm NewAlarm;
        std::string buffer;

        //std::cout << std::endl << "Start: " << std::endl;
        getline(file,NewAlarm.name);
        //std::cout <<std::endl<< NewAlarm.name << std::endl;

        getline(file,buffer);
        NewAlarm.id = std::stoi(buffer);
        //std::cout <<std::endl<< NewAlarm.id << std::endl;

        getline(file,NewAlarm.soundPath);
        //std::cout <<std::endl<< NewAlarm.soundPath << std::endl;

        getline(file,buffer);
        NewAlarm.time = std::stoi(buffer);
        //std::cout <<std::endl<< NewAlarm.time << std::endl;


        getline(file,buffer);
        NewAlarm.repeat = std::stoi(buffer);
        //std::cout <<std::endl<< NewAlarm.repeat << std::endl;

        getline(file,buffer);
        NewAlarm.enabled = std::stoi(buffer);
        //std::cout <<std::endl<< NewAlarm.enabled << std::endl;

        getline(file,buffer);
        NewAlarm.periodic = std::stoi(buffer);
        //std::cout <<std::endl<< NewAlarm.periodic<< std::endl;


        getline(file,buffer);
        NewAlarm = stringToDays(buffer,NewAlarm);
        //std::cout <<std::endl<< NewAlarm.days << std::endl;
        getline(file,buffer);

        for (Alarm &alarm : alarms) { // is there an alarm with the same id as Newalarm?
            if (NewAlarm.id  == alarm.id)
                duplicate = true;
        }
        if (!duplicate) {//if there isnt then add it to the deque
            alarms.push_front(NewAlarm);
        }
        duplicate = false;
    }

    std::sort(alarms.begin(), alarms.end(), [](const Alarm& a, const Alarm& b) {return a.time > b.time;}); //i need to learn about lambda expressions
    file.close();
    //std::cout << std::endl << "Amount of alarms is: " << alarms.size() << std::endl;
    //std::cout << std::endl << "AFTER SORTING, ORDER IS: " << std::endl;
    return true;
}


/// <summary>
///     Returns true if the current day is a day thats set as true in a "Alarm.days" struct
/// </summary>
/// <param name="alarm">Alarm info</param>
bool isDay (Alarm alarm) { // returns true if the current day is a enabled day
    chrono::time_point present = chrono::system_clock::now();
    time_t present_t = chrono::system_clock::to_time_t(present);

    tm timeContainer;
    localtime_r(&present_t, &timeContainer);

    //std::cout << "Current day: " << timeContainer.tm_wday << "\n";
    if (alarm.days[timeContainer.tm_wday] == true) // if day of week number (Sunday->Saturday) 0->6
        return true;
    return false;
}

/// <summary>
///     Returns true if the alarm is in the future
/// </summary>
/// <param name="alarm">Alarm info</param>
bool isFuture(Alarm alarm) {
    if (itoTime(alarm.time) > EvilPointToTime(chrono::system_clock::now()))
        return true;
    return false;
}


/// <summary>
///     Takes an int and returns the time as seconds since the start of the day as time_T
/// </summary>
/// <param name="time">Current time point</param>
///
time_t itoTime (int time) {

    int hours = time/100;
    int minutes = time%100;

    time_t evil = 60*((hours*60)+minutes); // this is evil
    return evil;
}


/// <summary>
///     Takes a time point and returns the amount of seconds since the start of the day as time_t
/// </summary>
/// <param name="time">Current time point</param>
///
time_t EvilPointToTime (chrono::system_clock::time_point time, bool debug) {

    time_t pctime = chrono::system_clock::to_time_t(currentTime(time));
    time_t evilTime = pctime%(60*60*24);

    if (debug) {
        std::cout << std::endl << "evil time: " << evilTime << "\n";
    }
    return evilTime;
}



/// <summary>
///     Takes a time point and returns another time point equal to the time point and its time zone displacement
/// </summary>
/// <param name="time">Current time point</param>
///
/*
  I didnt make this function, chatgpt did. you can tell by the morbillion autos
  I also now understand what people mean when they say chatgpt is dumb.
  I was trying to learn chrono by asking him ,but it kept regurgigating the first page of google search and using auto until i got mad
 */
chrono::system_clock::time_point currentTime (chrono::system_clock::time_point time) {
    auto displacement = chrono::current_zone();
    auto info = displacement ->get_info(chrono::system_clock::now());
    auto offset = info.offset;
    return time+offset;
}

/// <summary>
///     Takes a Alarm.days and turns it into a string that looks something like this 0100111
/// </summary>
/// <param name="alarm">Alarm that you wanna get the days from</param>
///
std::string daysToString(const Alarm &alarm) { // this function is broken
    std::string output = "";
    for (int idx=0; idx < 7; idx++) {
        std::cout <<"alarm day idx is: " << alarm.days[idx] << std::endl;
        output += (alarm.days[idx] == true ? "0" : "1"); // TODO: fix this, its outputting 0 no matter what
        std::cout <<std::endl << "State is: "<< alarm.days[idx] << "\n";
    }
    std::cout <<std::endl << "(function)Output is: "<< output << "\n";
    return output;
}

/// <summary>
///     Takes a string and turns it into Alarm.days
/// </summary>
/// <param name="alarm">Alarm that you wanna get the days from</param>
/// <param name="input">String containing the days</param>
///
Alarm stringToDays(std::string input, Alarm &alarm) {
    for (int idx=0; idx < 7; idx++) {
        alarm.days[idx] = input[idx];
    }
    return alarm;
}

/// <summary>
///     Checks if a file exists
/// </summary>
/// <param name="fileName">The name of the file that you wanna check</param>
bool DoesFileExist(std::string fileName) {
    std::ofstream file;
    file.open(fileName,std::ios::in);
    if (!file.is_open()) {
        file.close();
        return false;
    }
    file.close();
    return true;
}


/// <summary>
///     Handles the pomodoro, by default the duration of the pomodoro will be 40 minutes, and the rest will be 5 minutes
/// </summary>
/// <param name="attr">Queue attributes</param>
/// <param name="pomodoroDuration">Duration of the pomodoro, in seconds</param>
/// <param name="restingDuration">Duration of the rest, in seconds</param>
///
void PomodoroStart(mq_attr attr, int pomodoroDuration, int restingDuration) {

    enum state {
        studying,
        resting,
        paused,
        stopping,
        resuming
    };
    const std::string stopMessage = "STOP";
    const std::string pauseMessage = "PAUSE";
    const std::string resumeMessage = "RESUME";
    state pomodoroState = studying;
    state lastState = studying;
    bool resumed = false;
    bool recived = false;
    mq_unlink(POMODORO_QUEUE);
    int pomodoroEnd;
    int restingEnd;
    int timeInterval = 1;
    int currentTime;
    Message receivedMessage;
    int timeBuffer = 0;

    while (pomodoroState != stopping) { // state machine loop


        switch (pomodoroState) {
            case studying: {
                 recived =false;
                //mq_unlink(POMODORO_QUEUE);
                mqd_t mqd = mq_open(POMODORO_QUEUE, O_CREAT | O_RDONLY | O_NONBLOCK, 0600, &attr);
                if (mqd == -1) {
                    perror("Error opening queue");
                    exit(EXIT_FAILURE);
                }
                std::cout << "Studying" << std::endl;
                currentTime = EvilPointToTime(chrono::system_clock::now());
                if (resumed == true) {
                    pomodoroEnd = currentTime + timeBuffer; // if its resuming
                    resumed = false;
                }
                else {
                    pomodoroEnd = currentTime + pomodoroDuration;
                }
                for (int time =currentTime; time < pomodoroEnd; time+=timeInterval) { // handles waiting the time of the pomodoro and the change of state
                    std::this_thread::sleep_for(std::chrono::seconds(timeInterval));


                    mq_receive(mqd, reinterpret_cast<char *>(&receivedMessage), sizeof(receivedMessage), nullptr);// receive message and see if state has to change

                    if (strcmp(receivedMessage.msg,stopMessage.c_str()) == 0) {
                        mq_close(mqd);
                        recived =true;
                        pomodoroState = stopping;
                        break;
                    }
                    if (strcmp(receivedMessage.msg,pauseMessage.c_str()) == 0) {
                        mq_close(mqd);
                        recived =true;
                        timeBuffer = pomodoroEnd - time;
                        pomodoroState = paused;
                        break;
                    }
                }

                lastState = studying;
                if (!recived)
                    pomodoroState = resting;

            }
            break;
            case resting: {
                recived = false;
                // same as pomodoro but with resting duration instead of pomodoro duration
                //mq_unlink(POMODORO_QUEUE);
                mqd_t mqr = mq_open(POMODORO_QUEUE, O_CREAT | O_RDONLY | O_NONBLOCK, 0600, &attr);
                if (mqr == -1) {
                    perror("Error opening queue");
                    exit(EXIT_FAILURE);
                }
                std::cout << "Resting" << std::endl;
                Message restingMessage;
                currentTime = EvilPointToTime(chrono::system_clock::now());
                if (resumed == true) {
                    restingEnd = currentTime + timeBuffer; // if its resuming
                    resumed = false;
                }
                else {
                    restingEnd = currentTime + restingDuration;
                }
                for (int time = currentTime; time < restingEnd; time+=timeInterval) { // handles waiting the time of the resting and the change of state
                    std::this_thread::sleep_for(std::chrono::seconds(timeInterval));


                    mq_receive(mqr, reinterpret_cast<char *>(&restingMessage), sizeof(restingMessage), nullptr);// receive message and see if state has to change


                    if (strcmp(restingMessage.msg,stopMessage.c_str()) == 0) {
                        mq_close(mqr);
                        pomodoroState = stopping;
                        break;
                    }
                    if (strcmp(restingMessage.msg,pauseMessage.c_str()) == 0) {
                        mq_close(mqr);
                        timeBuffer = pomodoroEnd - time;
                        pomodoroState = paused;
                        break;
                    }
                    mq_close(mqr);
                }
                lastState = resting;
                if (!recived)
                    pomodoroState = studying;
            }
                break;
                case paused: {
                    std::cout << "Paused" << std::endl;
                    mqd_t queue = mq_open(POMODORO_QUEUE, O_CREAT | O_RDONLY, 0600, &attr);
                    mq_receive(queue, reinterpret_cast<char *>(&receivedMessage), sizeof(receivedMessage), nullptr);
                    if (strcmp(receivedMessage.msg,resumeMessage.c_str()) == 0) { // fix this, and also i need to recieve the message
                        pomodoroState = resuming;
                        mq_close(queue);
                    }
                }
                break;
            case stopping:// i dont think this will ever run? ima put it just in case
                lastState = pomodoroState;
                break;
            case resuming:
                std::cout << "Resuming" << std::endl;
                resumed = true;
                pomodoroState = lastState;
                lastState = resuming;
            std::this_thread::sleep_for(std::chrono::seconds(5));
                break;
            default:
                return;
            break;
        }
    }
}

/// <summary>
///     Sends the message STOP to POMODORO_QUEUE
/// </summary>
/// <param name="attr">Queue attributes</param>
///
void PomodoroStop(mq_attr attr) {
    mqd_t mqd = mq_open(POMODORO_QUEUE, O_CREAT | O_WRONLY, 0600, &attr);
    Message message;
    strcpy(message.msg, "STOP");
    mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 1);
    mq_close(mqd);
}
/// <summary>
///     Sends the message PAUSE to POMODORO_QUEUE
/// </summary>
/// <param name="attr">Queue attributes</param>
///
void PomodoroPause(mq_attr attr) {
    mqd_t mqd = mq_open(POMODORO_QUEUE, O_CREAT | O_WRONLY, 0600, &attr);
    const std::string pauseMessage = "PAUSE";
    Message message;
    strcpy(message.msg, pauseMessage.c_str());
    mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 1);
    mq_close(mqd);
}
/// <summary>
///     Sends the message RESUME to POMODORO_QUEUE
/// </summary>
/// <param name="attr">Queue attributes</param>
///
void PomodoroResume(mq_attr attr) {
    mqd_t mqd = mq_open(POMODORO_QUEUE, O_CREAT | O_WRONLY, 0600, &attr);
    Message message;
    strcpy(message.msg, "RESUME");
    mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 1);
    mq_close(mqd);
}

//i know i dont need a morbillion functions that do the same but i feel like its nicer to read if its like this (im just too lazy to change it)



/// <summary>
///     Handles alarms, but instead of just iterating a sleep() until time = closest alarm time, it creates a thread for every
///     alarm, and constantly waits for messages to ALARM_QUEUE, to communicate changes to those threads.
/// </summary>
/// <param name="attr">Queue attributes</param>
void BetterAlarmService(mq_attr attr) { // i added a stop but realistically you will just boot off your pc, it shoulnt really be an issue tho
    bool check = false;
    std::deque<unsigned int> deletedIds;
    std::deque <Alarm> alarms;
    mqd_t mqd = mq_open(ALARM_QUEUE, O_CREAT | O_RDONLY, 0600, &attr);
    Message receivedMessage;
    ReadAlarms(alarms);
    HandleAlarms(alarms, deletedIds);
    const std::string stopMessage = "STOP";
    const std::string newMessage = "NEW";
    const std::string deleteMessage = "DELETE";
    const std::string toggleMessage = "TOGGLE";

    while (true) {
        mq_receive(mqd, reinterpret_cast<char *>(&receivedMessage), sizeof(receivedMessage), nullptr);
        if (strcmp (receivedMessage.msg,stopMessage.c_str()) == 0) {
            for (Alarm &alarm : alarms) {
                alarm.enabled = false;
            }
            break;
        }
        if (strcmp (receivedMessage.msg,newMessage.c_str()) == 0) {
            std::cout << std::endl << std::endl << "THERES A NEW ALARM REEEEEEEE "<<std::endl;
            ReadAlarms(alarms);
            HandleAlarms(alarms, deletedIds);
        }
        else if (strcmp (receivedMessage.msg,deleteMessage.c_str()) == 0) {
            deletedIds.push_back(receivedMessage.id);
            std::cout << "Received DELETE for id: " << receivedMessage.id << std::endl;
        }
        else if (strcmp (receivedMessage.msg,toggleMessage.c_str()) == 0) {
            for (Alarm &alarm : alarms) {
                if (alarm.id == receivedMessage.id) {
                    alarm.enabled = !alarm.enabled;
                }
            }
        }
        else {
            std::cout << "Error, invalid message." << std::endl;
        }
    }


    HandleThreads(alarms);
}


/// <summary>
/// Creates a thread that will run the function ActivateAlarm() for every alarm thats in the future and its not handled
/// </summary>
/// <param name="alarms">A deque of type Alarm containing all alarms</param>
/// <param name="deletedIds">A deque of type unsigned int that has every alarm that has been deleted while the service is running</param>
void HandleAlarms(std::deque <Alarm> &alarms, std::deque<unsigned int> &deletedIds) {
    std::cout << std::endl<<"HANDLE ALARMS" << std::endl;
    for (Alarm &alarm : alarms) {
        if ((!alarm.handled) && (EvilPointToTime(chrono::system_clock::now()) < alarm.time)) {
            std::cout  << "Alarm time: " << alarm.time <<std::endl;
            alarm.thread = new std::thread(ActivateAlarm, std::ref(alarm), std::ref(deletedIds));
            alarm.handled = true;
        }
    }
}

/// <summary>
/// Gets current time and waits until alarm.time, then if the alarm is inside deletedIds, it disables it, and
/// and if the alarm is enabled, it sends a notification and plays the desired sound of the alarm.
/// </summary> TODO: repeat is done, but i wanna make it so that you can postpone it
/// <param name="alarm">Alarm data</param>
/// <param name="deletedIds">A deque containing the list of deleted alarms</param>
void ActivateAlarm(Alarm &alarm, std::deque <unsigned int> &deletedIds){
    Message message;
    std::string repeatMessage = "REPEAT";
    mqd_t mqd = mq_open(ALARM_QUEUE, O_CREAT | O_RDONLY, 0600, &attr);
    int currentTime = EvilPointToTime(std::chrono::system_clock::now());
    std::cout << std::endl << "THIS IS RUNNING";
    std::cout << std::endl << "Current time: " << currentTime << " seconds" << std::endl;
    std::cout  << "Alarm time: " << alarm.time <<std::endl;
    //std::cout << "Alarm name: " << alarm.name << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(alarm.time-currentTime));
    for (unsigned int id : deletedIds) {
        if (alarm.id == id)
            alarm.enabled = false;
    }
    if (alarm.enabled) {
        while (strcmp (message.msg,repeatMessage.c_str()) != 0) {
            mq_receive(mqd, reinterpret_cast<char *>(&message), sizeof(message), nullptr);
            SendNotification(alarm.name, "This is your alarm");
            PlaySound(alarm.soundPath); // only issue is that uh paplay doesnt stop until the whole sound plays and i dont know how to stop it so itll stop repeating after it plays at least once
        }
    }
}

/// <summary>
/// Waits for every alarm thread to finish their task. Realistically i dont think this will ever run
/// </summary>
/// <param name="alarms">A deque containing every alarm</param>
void HandleThreads(std::deque<Alarm> &alarms) {
    for (Alarm alarm : alarms) {
        alarm.thread->join();;
    }
}

//TODO: now i need to make it so that i can delete alarms, or disable them. so i need to make it so that i can run another process that disables x alarm, so i should probably give every alarm an id?

/// <summary>
/// Generates a random unsigned int
/// </summary>
/// <param name="min">minimum number, default value is 0</param>
/// <param name="max">maximum number, default value is the max number a 16 bit unsigned int can reach</param>
unsigned int randInt(int min, int max) { //to make the alarm ids
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(min, max);
    return dis(gen);
}

/// <summary>
///     Sends "TOGGLE" to the ALARM_QUEUE and the alarm id
/// </summary>
/// <param name="attr">queue attributes</param>
/// <param name="id">alarm id</param>
void toggleAlarm(mq_attr attr, unsigned int id) { //wip
    mqd_t mqd = mq_open(ALARM_QUEUE, O_CREAT | O_WRONLY, 0600, &attr);
    Message message;
    strcpy(message.msg, "TOGGLE");
    message.id = id;
    mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 0);
    mq_close(mqd);
}
/// <summary>
///     Sends "DELETE", and ID to the ALARM_QUEUE, reads all alarms, deletes from memory the alarm with id "id"
///     then deletes all the alarms and writes all the alarms again without the deleted one.
/// </summary>
/// <param name="attr">queue attributes</param>
/// <param name="id">alarm id</param>
void deleteAlarm(mq_attr attr, unsigned int id) { //wip
    std::deque <Alarm> alarms;
    int a = 0;
    bool found = false;
    ReadAlarms(alarms);
    for (Alarm &alarm : alarms) {
        if (alarm.id == id) {
            found = true;
            break;
        }
        a++;
    }
    if (found) {
        alarms.erase(alarms.begin() + a);
    clearAlarms();
    for (Alarm &alarm : alarms) {
        WriteAlarm(alarm);
    }
    }
    else {
        return;
    }
    Message message;
    mqd_t mqd = mq_open(ALARM_QUEUE, O_CREAT | O_WRONLY, 0600, &attr);
    strcpy(message.msg, "DELETE");
    message.msg[9] = '\0';
    message.id = id;
    mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 1);
    mq_close(mqd);
}

/// <summary>
///     Sends "NEW" to the ALARM_QUEUE
/// </summary>
/// <param name="attr">queue attributes</param>
void callNewAlarm(mq_attr attr) {
    Message message;
    mqd_t mqd = mq_open(ALARM_QUEUE, O_CREAT | O_WRONLY, 0600, &attr);
    strcpy(message.msg, "NEW");
    message.msg[9] = '\0';
    message.id = 0;
    mq_send(mqd, reinterpret_cast<char *>(&message), sizeof(message), 1);
    mq_close(mqd);
}

/// <summary>
///     Deletes the file "Alarm.txt"
/// </summary>
void clearAlarms(void) {
    const std::string filename = "Alarm.txt";
    std::filesystem::remove(filename);
}
