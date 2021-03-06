﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Current_Cycling_Controls {
    public class U {

        public static int TDKBaudRate = 57600;
        public static string COMPort = "COM3";
        public static string VoltageCompliance = "60"; //Kat's code
        public static string SampleTxtHeader = "Cycle Number,Epoch Time (seconds),Total Time (hrs),Time into Cycle (min),Current Status,Sample Name,Current (A),Voltage (V),# Cells,Cell VoC,TempSensor,SetCurrent,Estimated Rs,Temp 1,Temp 2,Temp 3,Temp 4,Temp 5,Temp 6,Temp 7,Temp 8,Temp 9,Temp 10,Temp 11,Temp 12,Temp 13,Temp 14,Temp 15,Temp 16,SmokeVoltage 1,SmokeVoltage 2,SmokeVoltage 3,SmokeVoltage 4,SmokeVoltage 5,SmokeVoltage 6,SmokeVoltage 7,SmokeVoltage 8,SmokeLevel 1,SmokeLevel 2,SmokeLevel 3,SmokeLevel 4,SmokeLevel 5,SmokeLevel 6,SmokeLevel 7,SmokeLevel 8";
        public static int ArduinoPacketSize = 36;
        public static int ResultsSaveTimeON = 2000;
        public static int ResultsSaveTimeOFF = 20000;
        public static int MaxVoltageBurning = 53;

        public static string CCdb = "CurrentCycling";
        public static string CCTable = "Cycling";
        public static string CCRecipeTable = "CCRecipe";

        

        public enum CmdType {
            None,
            Sequence,
            StartCycling,
            UpdateUI,
            StopCycling,
            CleanGUI,
            RecievedPacket,
            UpdateHeartBeatPacket,
            CheckConnection,
            ArduinoConnectSuccess
        }

        public enum GUIReport {
            None,
            ReEnableGUI,
            RecievedPacket,
            UpdateHeartBeat,
            UpdateTDKConnection,
            UpdateTDKVals,
            Testing
        }

        public static class Logger {
            public static StringBuilder LogString = new StringBuilder();
            public static int maxBytes = 20000000; // 20mb
            public static int maxChars = maxBytes / sizeof(char);
            public static string Dir = "C:/CCLogs/";

            public static void WriteLine(string str) {
                Console.WriteLine(str);
                LogString.Append(str).Append(Environment.NewLine);
            }

            public static void Write(string str) {
                Console.Write(str);
                LogString.Append(str);
            }

            public static void SaveLog(bool append = false) {
                if (!Directory.Exists(Dir)) {
                    Directory.CreateDirectory(Dir);
                }

                int bytess = LogString.ToString().Length * sizeof(Char) + sizeof(int);

                // if greater than 20mb split it up and save seperately
                if (bytess > maxBytes) {
                    var log = LogString.ToString();
                    List<string> strList = new List<string>();
                    for (int i = 0; i < log.Length; i += maxChars) {
                        if ((i + maxChars) < log.Length)
                            strList.Add(log.Substring(i, maxChars));
                        else
                            strList.Add(log.Substring(i));
                    }

                    int j = 0;
                    foreach (var s in strList) {
                        string path = $"{Dir}Log {DateTime.Now.ToString("yy_MM_dd_H_mm")}_{j}.txt";
                        if (LogString != null && LogString.Length > 0) {
                            using (StreamWriter file = new StreamWriter(path)) {
                                file.Write(s);
                                file.Close();
                                file.Dispose();
                            }
                        }
                        j++;
                    }
                }
                else {
                    var path = $"{Dir}Log {DateTime.Now.ToString("yy_MM_dd_H_mm")}.txt";
                    if (LogString != null && LogString.Length > 0) {
                        using (StreamWriter file = new StreamWriter(path)) {
                            file.Write(LogString.ToString());
                            file.Close();
                            file.Dispose();
                        }
                    }
                }

                LogString.Clear();
            }
        }


        public enum Status {
            Error = -1,
            Initialize,

        }

        public enum Results {
            CycleNum,
            Epoch,
            TotalHrs,
            TotalCycleTime,
            BiasStatus,
            SampleName,
            Current,
            Voltage,
            NumCells,
            Voc,
            TempSensors,
            SetCurrent,
            Rs, 
            Temp1,
            Temp2,
            Temp3,
            Temp4,
            Temp5,
            Temp6,
            Temp7,
            Temp8,
            Temp9,
            Temp10,
            Temp11,
            Temp12,
            Temp13,
            Temp14,
            Temp15,
            Temp16,
        }

        public string GetCOMPort(string adrs) {
            switch (adrs) {
                case "01":
                    return "COM01";
                case "02":
                    return "COM01";
                case "03":
                    return "COM01";
                case "04":
                    return "COM01";
                case "05":
                    return "COM01";
                case "06":
                    return "COM01";
                case "07":
                    return "COM01";
                case "08":
                    return "COM01";
                case "09":
                    return "COM01";
                case "10":
                    return "COM01";
                case "11":
                    return "COM01";
                case "12":
                    return "COM01";
                default:
                    return "";
            }
        }


    }
}
