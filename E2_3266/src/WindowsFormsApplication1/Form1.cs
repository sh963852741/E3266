using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using System.IO.Ports;

namespace WindowsFormsApplication1
{
    public partial class Form1 : Form
    {
        private SerialPort mySerialPort;
        public Form1()
        {
            InitializeComponent();
        }

        void OnDataReceived(Object sender, SerialDataReceivedEventArgs e)
        {
            string message = "[RECV " + DateTime.Now.ToString() + "] " + ((SerialPort)sender).ReadLine() + "\n";
            richTextBox.SelectionColor = Color.Blue;
            richTextBox.AppendText(message);
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            Control.CheckForIllegalCrossThreadCalls = false;
            mySerialPort = new SerialPort();
            string[] portNameArr = SerialPort.GetPortNames();
            if (portNameArr.Length == 0)
            {
                portBox.Enabled = false;
                sendButton.Enabled = false;
            }
            else
            {
                portBox.Items.AddRange(portNameArr);
                portBox.Text = portNameArr[0];
                mySerialPort.PortName = portNameArr[0];
                mySerialPort.Open();
            }    
            mySerialPort.DataReceived += new SerialDataReceivedEventHandler(OnDataReceived);
        }

        private void sendButton_Click(object sender, EventArgs e)
        {
            string message = "[SENT " + DateTime.Now.ToString() + "] " + inputBox.Text + "\n";
            this.mySerialPort.Write(message);
            richTextBox.SelectionColor = Color.Green;
            richTextBox.AppendText(message);
            inputBox.Text = string.Empty;
        }

        private void portBox_SelectionChangeCommitted(object sender, EventArgs e)
        {
            mySerialPort.Close();
            mySerialPort.PortName = ((ComboBox)sender).Text;
            try
            {
                mySerialPort.Open();
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message, ex.GetType().ToString() + " 类型的异常",MessageBoxButtons.OK,MessageBoxIcon.Warning);
            }
        }

    }
}
