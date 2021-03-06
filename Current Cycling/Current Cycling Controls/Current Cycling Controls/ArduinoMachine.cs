﻿using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.ComponentModel;

namespace Current_Cycling_Controls {
    public delegate void ArduinoEvent(object sender, GUIArgs e);
    public class ArduinoMachine {
        private SerialPort _serArduino;
        private BackgroundWorker _readThread = new BackgroundWorker();
        private readonly Queue<CoreCommand> _commandQueue = new Queue<CoreCommand>();
        private readonly AutoResetEvent _commReset = new AutoResetEvent(false);
        private readonly object _lock = new object();
        public RecievePacket _recievedPacket;
        public event CoreCommandEvent NewCoreCommand;
        public TransmitPacket _transmitPacket;
        public bool Connected;
        public bool _printPackets;
        public ArduinoMachine() {
            _transmitPacket = new TransmitPacket("200","200","85","25","0","0","0000000000000000","00000000", "0");
            Connected = false;
        }

        public void StartArduinoMachine() {
            _serArduino = new SerialPort();
            OpenPorts();
            while (true) {
                try {
                    ReadPackets();
                    NewCoreCommand?.Invoke(this, new CoreCommand() { Type = U.CmdType.RecievedPacket });
                }
                catch (Exception ex) {
                    if (ex is TimeoutException) {
                        U.Logger.WriteLine(ex.ToString());
                    }
                    else if (ex is InvalidPacketSize) {
                        U.Logger.WriteLine(ex.ToString());
                    }
                    else {
                        U.Logger.WriteLine($"{ex}");
                    }
                }
                SendPackets();
            }
        }
        

        private void OpenPorts() {
            string[] ports = SerialPort.GetPortNames();
            _serArduino.Close();
            U.Logger.WriteLine($"Connecting to Arduino");
            // loop through each port forever until we get the correct arduino packet
            while (!Connected) {
                _serArduino.BaudRate = 115200;
                _serArduino.NewLine = "\r";
                _serArduino.ReadTimeout = 2000;
                foreach (var port in ports) {
                    try {
                        _serArduino.PortName = port;
                        _serArduino.Open();
                        _serArduino.DiscardOutBuffer();
                        _serArduino.DiscardInBuffer();
                        var str = _serArduino.ReadLine().Split(',').Select(sValue => sValue.Trim()).ToList();
                        U.Logger.WriteLine($"{str.Count}");
                        if (str.Count == U.ArduinoPacketSize) {
                            _serArduino.DiscardOutBuffer();
                            _serArduino.DiscardInBuffer();
                            Connected = true;
                            NewCoreCommand?.Invoke(this, new CoreCommand() { Type = U.CmdType.ArduinoConnectSuccess });
                            U.Logger.WriteLine($"Arduino connection successful {port}");
                            break;
                        }
                    }
                    catch (Exception exc) {
                        //U.Logger.WriteLine($"{exc}");
                        _serArduino.Close();
                    }
                }
            }
        }

        private void ReadPackets() {
            _serArduino.DiscardInBuffer();
            var packet = _serArduino.ReadLine();
            if (_printPackets) U.Logger.WriteLine($"{packet}");
            _recievedPacket = ParsePacket(packet);
        }

        private void SendPackets() {
            _serArduino.WriteLine(_transmitPacket.ToStringPacket(_printPackets));
        }


        private RecievePacket ParsePacket(string packet) {
            var values = packet.Split(',').Select(sValue => sValue.Trim()).ToList();
            if (values.Count != U.ArduinoPacketSize) {
                throw new InvalidPacketSize(values.Count);
            }
            return new RecievePacket(values.Take(16).ToList(), values.Skip(16).Take(8).ToList(),
                values[24], values[25], values[26], values[27]);
        }

        public void UpdateTransmit(TransmitPacket t, bool printPackets) {
            _transmitPacket = t;
            _printPackets = printPackets;
        }


    }

    public class RecievePacket{
        public List<double> TempList { get; set; }
        public List<double> SmokeList { get; set; }
        public bool TempAlarm { get; set; }
        public bool SmokeAlarm { get; set; }
        public bool EMSSTOP { get; set; }
        public bool HeartBeatGood { get; set; }

        public RecievePacket(List<string> temps, List<string> smokes, string tempAlarm,
            string smokeAlarm, string emsSTOP, string heartGood) {
            TempList = temps.Select(Double.Parse).ToList();
            SmokeList = smokes.Select(Double.Parse).ToList();
            TempAlarm = Convert.ToBoolean(Int32.Parse(tempAlarm));
            SmokeAlarm = Convert.ToBoolean(Int32.Parse(smokeAlarm));
            EMSSTOP = Convert.ToBoolean(Int32.Parse(emsSTOP));
            HeartBeatGood = Convert.ToBoolean(Int32.Parse(heartGood));

        }
    }

    public class TransmitPacket {
        private string TempSetPoint { get; set; }
        private string SmokeSetPoint { get; set; }
        private string BiasCurrentONTemp { get; set; }
        private string BiasCurrentOFFTemp { get; set; }
        private string BiasCurrentStatus { get; set; }
        private string PauseFans { get; set; }
        private string ActiveTemps { get; set; }
        private string ActiveSmokes { get; set; }
        private string CurrentCycling { get; set; }
            
        public TransmitPacket(string setTemp, string setSmoke, string biasONTemp,
            string biasOFFTemp, string currentStatus, string pause, string activetemps, string activesmokes, string cycling) {
            TempSetPoint = setTemp;
            SmokeSetPoint = setSmoke;
            BiasCurrentONTemp = biasONTemp;
            BiasCurrentOFFTemp = biasOFFTemp;
            BiasCurrentStatus = currentStatus;
            PauseFans = pause;
            ActiveTemps = activetemps;
            ActiveSmokes = activesmokes;
            CurrentCycling = cycling;

        }

        public string ToStringPacket(bool print) {
            string[] str = { TempSetPoint, SmokeSetPoint, BiasCurrentONTemp, BiasCurrentOFFTemp,
                BiasCurrentStatus, PauseFans, ActiveTemps, ActiveSmokes, CurrentCycling};
            string strr = string.Join(",", str);
            strr = strr.Insert(0, "<");
            strr += ">";
            if (print) U.Logger.WriteLine($"{strr}");
            return strr;
        }

    }

    class InvalidPacketSize : Exception {
        public InvalidPacketSize(int count) {
            U.Logger.WriteLine($"Invalid Packet Structure! Got only {count} entries");
        }
    }

}
