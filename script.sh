#!/usr/bin/env bash
#echo "Today is " "$(date)" #isntead of backticks use $()
# $1 $2 $3 are the args you run the script with


case $# in
  0)
    echo "no arguments were given"
mode=$(echo "Pomodoro \nAlarm \nRemindme \nChronometer" | rofi -dmenu -p "testing")
#echo $selection
case $mode in
  "Pomodoro ")
    echo "you selected: Pomodoro"
    pselection=$(echo "Start\nStop\nPause\nResume" | rofi -dmenu -p "Choose:")
    case $pselection in
    "Start")
    pcommand="s"
    ;;
    "Stop")
    pcommand="o"
    ;;
    "Pause")
    pcommand="p"
    ;;
    "Resume")
    pcommand="r"
    ;;
    esac
    echo "-P"pcommand
    ./Alarm "-P""$pcommand"
  ;;
  #####################################################################################
  "Alarm ") # TODO     // format will be -a (if -c =>"customsoundpath") "alarmname" -(r || -R) -(u || p)
    echo "you selected: Alarm"
    sound=$(echo "Yes\nNo" | rofi -dmenu -p "Do you want a custom sound?")
    case $sound in
      "Yes")
      
      path=$(./Alarm -i | rofi -dmenu -P "Choose a file")
      soundpath=-c $path
      echo "\n\n\n\n"
      echo $path
      ;;
      "No")
      soundpath=""
      ;;
      *)
        echo error1
        ;;
    esac
    repeat=$(echo "Yes\nNo" | rofi -dmenu -p "Repeat?")
    case $repeat in
          "Yes")
          rpt="-R"
          ;;
          "No")
          rpt="-R"
          ;;
          *)
            echo Error
            ;;
        esac
        echo "Repeat is" rpt
    name=$(rofi -dmenu -p "Alarm name")
    atype=$(echo "Unique\nPeriodic" | rofi -dmenu -p "What alarm type?")
    case $atype in
    "Unique")
      at="-u"
    ;;
    "Periodic")
      at="-p"
      ##TODO: handle periodic
      days=$(echo "" | rofi -dmenu -p "What days? (Sunday to Monday In binary)")
    ;;
    esac
    time=$(rofi -dmenu -p "Time")
    ./Alarm -a $soundpath $name $rpt $at $days $time
    ;;# TODO     // format will be -a (if -c =>"customsoundpath") "alarmname" -(r || -R) -(u || p)
  #####################################################################################
  "Remindme ")
    #echo "you selected: remindme"
    unit=$(echo "Hours \nMinutes \nSeconds" | rofi -dmenu -p "What time unit?")
     case $unit in
      "Hours ")
      u="-h"
      ;;
      "Minutes ")
      u="-m"
      ;;
      "Seconds")
      u="-s"
      ;;
    esac
    time=$(echo "" | rofi -dmenu -p "How much time?")
    #echo $time $u
    ./Alarm -R "$time" "$u" # Call the pomodoro with the selected parameters
    ;;
  #####################################################################################
  "Chronometer")
   #echo "you selected: Chronometer"
    cmode=$(echo "Start\nStop" | rofi -dmenu -p "Chronometer")
      case $cmode in
        "Start")
        cparam="-C"
        ;;
      "Stop")
        cparam="-c"
        ;;
      esac
      ./Alarm "$cparam"
    ;;
  *)
    echo "error"
  esac
  #####################################################################################
#bla=$(echo "" | rofi -dmenu -p "Enter Text > ") text input
#read answer
#echo "Text is: " $bla
;;
*) # else
  case $1 in
  #####################################################################################
  "-A")
      sound=$(echo "Yes\nNo" | rofi -dmenu -p "Do you want a custom sound?")
      case $sound in
        "Yes")

        path=$(./Alarm -i | rofi -dmenu -P "Choose a file")
        soundpath=-c $path
        echo "\n\n\n\n"
        echo $path
        ;;
        "No")
        soundpath=""
        ;;
        *)
          echo error1
          ;;
      esac
      repeat=$(echo "Yes\nNo" | rofi -dmenu -p "Repeat?")
      case $repeat in
            "Yes")
            rpt="-R"
            ;;
            "No")
            rpt="-R"
            ;;
            *)
              echo Error
              ;;
          esac
          echo "Repeat is" rpt
      name=$(rofi -dmenu -p "Alarm name")
      atype=$(echo "Unique\nPeriodic" | rofi -dmenu -p "What alarm type?")
      case $atype in
      "Unique")
        at="-u"
      ;;
      "Periodic")
        at="-p"
        ##TODO: handle periodic
        days=$(echo "" | rofi -dmenu -p "What days? (Sunday to Monday In binary)")
      ;;
      esac
      time=$(rofi -dmenu -p "Time")
      ./Alarm -a $soundpath $name $rpt $at $days $time
  ;;
  "-u") # unique alarm with full options
  echo "running unique alarm"

      sound=$(echo "Yes\nNo" | rofi -dmenu -p "Do you want a custom sound?")
      case $sound in
        "Yes")

        path=$(./Alarm -i | rofi -dmenu -P "Choose a file")
        soundpath=-c $path
        echo "\n\n\n\n"
        echo $path
        ;;
        "No")
        soundpath=""
        ;;
        *)
          echo error1
          ;;
      esac
      repeat=$(echo "Yes\nNo" | rofi -dmenu -p "Repeat?")
      case $repeat in
            "Yes")
            rpt="-R"
            ;;
            "No")
            rpt="-r"
            ;;
            *)
              echo Error
              ;;
          esac
          echo "Repeat is" rpt
      name=$(rofi -dmenu -p "Alarm name")
      at="-u" # alarm type will be unique
      time=$(rofi -dmenu -p "Time")
      ./Alarm -a $soundpath $name $rpt $at $days $time
  ;;
  #####################################################################################
  "-p") #Periodic alarm with full options
    sound=$(echo "Yes\nNo" | rofi -dmenu -p "Do you want a custom sound?")
        case $sound in
          "Yes")
          path=$(./Alarm -i | rofi -dmenu -P "Choose a file")
          soundpath=-c $path
          echo "\n\n\n\n"
          echo $path
          ;;
          "No")
          soundpath=""
          ;;
          *)
            echo error1
            ;;
        esac
        repeat=$(echo "Yes\nNo" | rofi -dmenu -p "Repeat?")
        case $repeat in
              "Yes")
              rpt="-R"
              ;;
              "No")
              rpt="-R"
              ;;
              *)
                echo Error
                ;;
            esac
            echo "Repeat is" rpt
          name=$(rofi -dmenu -p "Alarm name")
          days=$(echo "" | rofi -dmenu -p "What days? (Sunday to Monday In binary)")
        time=$(rofi -dmenu -p "Time")
        ./Alarm -a $soundpath $name $rpt $at $days $time
  ;;
  "-f") #time input only, unique alarm. Fast.
      echo "Running fast alarm."
      soundpath=""
      if [ "$2" = "-R" ] ## if theres no second value inputed (usage is -f and optional -R, if anything else is inputted itll just use -r)
      then
        rpt="-R"
      else
        rpt="-r"
      fi
      name="FastAlarm"
      at="-u"
      time=$(rofi -dmenu -p "Time")
      ./Alarm -a $soundpath $name $rpt $at $days $time
  ;;
  esac
  ;;
esac