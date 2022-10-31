/*******************************************************************

  1 Start het programma

  2 Als er een mixer gevonden wordt , dan word de kolom achtergroend groen.

  3 Als de volgorde niet klopt met wat er in de kolom staat wordt het register aangepast voor de USB audio volgorde.
    hiervoor zal de computer 2 keer opnieuw moeten worden opgestart dmv dit programmaatje



  ClearReset.bat   Deze batch file gooit alle audio PID codecs eruit dmv uvd.exe <---- is USBdeview

  Reboot.bat       Deze batch file reboot de PC na 5 seconden

  mixers.xml       Hierin staan alle mixer usb volgorde in opgeslagen

  Settings.xml     Houdt bij hoeveel keer de PC is opgestart

  mixersbk.xml     Om een default volgorde terug te kunnen zetten

********************************************************************/


using OrderUSB;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml;

namespace USBorder
{
    public partial class Form1 : Form
    {
        static string path = Directory.GetCurrentDirectory();
        static string programFilePat = path;
        static string programFilePath = programFilePat + "\\";
        static int timerCount = 0;
        int corrected = 0;





        public Form1()
        {
            InitializeComponent();
        }




        // FORM1_LOAD --------------------------------------------------------------------------------------------
        private void Form1_Load(object sender, EventArgs e)
        {

            timer1.Enabled = false;
            listBox1.HorizontalScrollbar = true;
            listBox1.RightToLeft = RightToLeft.No;

            
            DataSet dataSet = new DataSet();

            // Bestaat het bestand wel ?
            bool fileExists = (System.IO.File.Exists(programFilePath + "mixers.xml") ? true : false);

            if (fileExists)
            {
                // Load all xml data into dataset
                dataSet.ReadXml(programFilePath + "mixers.xml");
                // Load all dataset data into dataGridView1.DataSource
                dataGridView1.DataSource = dataSet.Tables[0];
               dataGridView1.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
            }
            else
            {
                MessageBox.Show("File " + programFilePath + "mixers.xml" + " not found");
                Application.Exit();
            }


            //Detecteer vanuit het register, welk type mixer er aangesloten is.

            MixerName mixer = new MixerName();

            string mixertype = mixer.TypeOfMixer(programFilePath);

            if (mixertype != null)
            {
                dataGridView1.Columns[mixertype].DefaultCellStyle.BackColor = Color.LightGreen;
                //dataGridView1.Columns[3].DefaultCellStyle.BackColor = Color.LightGreen;
            }

            // Kollommen mogen niet gesorteerd kunnen worden !!
            foreach (DataGridViewColumn column in dataGridView1.Columns)
            {
                column.SortMode = DataGridViewColumnSortMode.NotSortable;
            }

            // Rows nummering
            dataGridView1.RowHeadersWidth = 50;
            for (int i = 0; i < dataGridView1.Rows.Count; i++)
            {
                dataGridView1.Rows[i].HeaderCell.Value = (i + 1).ToString();
            }
            int tryAgain = ReadXmlSettings("corrected");

            if (tryAgain==2)
            {
                button1.Text = "Second Start";
            }

        }






        // START BUTTON ---------------------------------------------------------------------------------------------
        public void Button1_Click(object sender, EventArgs e)
        {
            //Lees uit het datagridview welke namen er in de array gezet moeten worden. 
            MixerName mixer = new MixerName();
            string mx = mixer.TypeOfMixer(programFilePath);
            if (mx == null)
            {
                MessageBox.Show("No mixers found\n Exiting  application");
                Application.Exit();
            }

            //Traceer in welke kolom je moet zijn
            int typeMix = 0;
            for (int t = 0; t < dataGridView1.Columns.Count; t++)
            {
                if ((string)dataGridView1.Columns[t].Name == mx)
                {
                    typeMix=t;
                }
            }

            //Lees de kolom met de cellen in de array 
            string[] GridArray = new string[dataGridView1.Rows.Count];
            for (int t = 0; t < dataGridView1.Rows.Count; t++)
            {
                GridArray[t] = (string)dataGridView1.Rows[t].Cells[typeMix].Value;
            }

            //Kloppen de namen uit de Array met wat er in het register staat (wrong ?)
            WrongKeyDetected wrong = new WrongKeyDetected();
            int wrongkey = wrong.WrongKey(GridArray);


            // lees de reboot stand uit readSettings.xml
            corrected = ReadXmlSettings("corrected");



            if (wrongkey == 1 || corrected > 0)
            {
                
                listBox1.Items.Add("Fail ! PCM2900 Registry keys are not the same as the XML table, Flag = " + (wrongkey));
                listBox1.Items.Add(mixer.TypeOfMixer(programFilePath) + " Detected ");
                //button1.Text = "Try again";
            }
            else
            {
                listBox1.Items.Add("Success ! PCM2900 Registry keys are the same as the XML table, Flag = " + (wrongkey));
                listBox1.Items.Add(mixer.TypeOfMixer(programFilePath) + " Detected ");
                //button1.Text = "Finished";
            }

            // Voorkeerde registersleutel gevonden !
            if (wrongkey > 0)
            {
                corrected++;
                //corrected2File(corrected);
                Corrected2xmlFile(corrected);
            }

            // Geen voorkeerde registersleutel gevonden
            if (wrongkey == 0)
            {
                corrected = 0;
                //corrected2File(corrected);
                Corrected2xmlFile(corrected);
                ProcessStartInfo proc = new ProcessStartInfo
                {
                    FileName = "cmd",
                    WindowStyle = ProcessWindowStyle.Normal,
                    Arguments = "/C mmsys.cpl"  // Start windows Audio windowtje , audio settings
                };
                Process.Start(proc);
                button1.Text = "Finished";
            }

            // Houd bij hoeveel keer de pc opnieuw moet worden opgestart
            // clear de register sleutels en start de pc opnieuw op voor de 1e keer !
            if (corrected == 1)
            {
                corrected++;
                ClearRegKeys();
                Corrected2xmlFile(corrected);
            }

            // Houd bij hoeveel keer de pc opnieuw moet worden opgestart
            // Zet de namen voor de kanalen goed en start de pc voor de tweede keer opnieuw op.
            else if (corrected > 1)
            {
                corrected = 0;
                Corrected2xmlFile(corrected);
                CorrigeerRegister corrReg = new CorrigeerRegister();
                corrReg.CorrigeerReg(GridArray);
                Reboot();
            }

        }







        // SAVE BUTTON -------------------------------------------------------------------------------------------
        private void Button2_Click(object sender, EventArgs e)
        {
            DataTable dt = new DataTable
            {
                TableName = "USBnames"//give it a name
            };//create the data table

            //Maak het aantal kollommen aan
            for (int i = 0; i < dataGridView1.Columns.Count; i++)
            {
                dt.Columns.Add(dataGridView1.Columns[i].HeaderText);
            }

            //Loop door iedere cel van de DataGridView
            foreach (DataGridViewRow currentRow in dataGridView1.Rows)
            {
                dt.Rows.Add();
                int runningCount = 0;
                //loop bij iedere regel door elke kolom 
                foreach (DataGridViewCell item in currentRow.Cells)
                {
                    dt.Rows[dt.Rows.Count - 1][runningCount] = item.FormattedValue;
                    runningCount++;
                }
            }

            if (dt != null)
            {
                dt.WriteXml(programFilePath + "mixers.xml");
            }

            dataGridView1.RowHeadersWidth = 50;
            for (int i = 0; i < dataGridView1.Rows.Count; i++)
            {
                dataGridView1.Rows[i].HeaderCell.Value = (i + 1).ToString();
            }
            MessageBox.Show("Saved");
        }






        // RESET BUTTON zet de standaard namen terug---------------------------------------------------------------------
        private void Button3_Click(object sender, EventArgs e)
        {
            // Bestaat het bestand wel ?
            bool fileExists = (System.IO.File.Exists(programFilePath + "mixersbk.xml") ? true : false);

            if (fileExists)
            {
                DataSet dataSet = new DataSet();
                dataSet.ReadXml(programFilePath + "mixersbk.xml");
                dataGridView1.DataSource = dataSet.Tables[0];
                dataGridView1.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
                dataGridView1.RowHeadersWidth = 50;
                for (int i = 0; i < dataGridView1.Rows.Count; i++)
                {
                    dataGridView1.Rows[i].HeaderCell.Value = (i + 1).ToString();
                }
            }
            else
            {
                MessageBox.Show("File " + programFilePath + "mixersbk.xml" + " not found");
                Application.Exit();
            }
        }






        

        // TEST BUTTON -----------------------------------------------------------------------------------
        private void Button4_Click(object sender, EventArgs e)
        {
            timer1.Enabled = true;
        }






        // CANCEL BUTTON ---------------------------------------------------------------------------------
        private void Button5_Click(object sender, EventArgs e)
        {
            timer1.Enabled = false;
            button5.Text = Convert.ToString("Cancel ");
            timerCount = 0;
        }




        // -------------------------------------------------------------------------------------------------------
        // -------------------------------------------------------------------------------------------------------
        // -------------------------------------------------------------------------------------------------------
        // -------------------------------------------------------------------------------------------------------


        // ################ Losse functie dat : Batch file ClearReset.bat Start ############################################
        private void ClearRegKeys()
        {
            System.Diagnostics.Process process = new System.Diagnostics.Process();
            process.StartInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Normal;
            process.StartInfo.FileName = programFilePath + "ClearReset";
            process.Start();
        }



        // ################ Losse functie dat : Timer1 doet starten en de PC doet rebooten #################################
        private void Reboot()
        {
            timer1.Enabled = true;
        }





        // ################ Losse functie dat : In corrected.txt de waarde w wegschrijft ###################################
        private void Corrected2File(int w)
        {
            StreamWriter wrl = new StreamWriter(programFilePath + "corrected.txt");
            wrl.WriteLine(w);
            wrl.Close();
        }



        // ################# Losse functie dat : Aangeeft of het gecorriceerd is corr=0, 1 , 2 ##############################
        private int ReadXmlSettings(string corrected)
        {

            // Bestaat het bestand wel ?
            bool fileExists = (System.IO.File.Exists(programFilePath + "Settings.xml") ? true : false);
            int corr = 0;
            if (fileExists)
            {

                XmlDocument XmlDoc = new XmlDocument(); // 
                XmlDoc.Load(programFilePath + "Settings.xml");
                XmlNodeList nodeList = XmlDoc.GetElementsByTagName("corrected");

                for (int i = 0; i < nodeList.Count; i++)
                {
                    if (nodeList[i].InnerText.Length > 0)
                    {
                        corr = Convert.ToInt32(nodeList[i].InnerText);
                    }
                }
            }
            else
            {
                MessageBox.Show("File " + programFilePath + "Settings.xml" + " not found");
                Application.Exit();
            }
            return corr;
        }





        // ############## Losse functie dat : waardes naar Settings.xml schrijft ##################################################
        private void Corrected2xmlFile(int corrected)
        {
            DataTable dt3 = new DataTable
            {
                TableName = "Settings"//give it a name
            };//create the data table
            dt3.Rows.Add();
            dt3.Columns.Add("comboBox1SelectedIndex");
            dt3.Columns.Add("corrected");
            dt3.Rows[0][1] = corrected;
            dt3.Columns.Add("willem");
            dt3.Rows[0][2] = "Post";

            if (dt3 != null)
            {
                dt3.WriteXml(programFilePath + "Settings.xml");
            }
        }





        // ############ Losse functie dat : de PC laat rebooten na 10 Sec. ##############################################
        private void Timer1_Tick(object sender, EventArgs e)
        {
            timerCount++;
            button5.Text = Convert.ToString("Reboot in 10 sec " + timerCount + " Cancel ");
            if (timerCount > 9)
            {
                ProcessStartInfo proc = new ProcessStartInfo
                {
                    FileName = "cmd",
                    WindowStyle = ProcessWindowStyle.Normal,
                    Arguments = "/C shutdown " + "-f -r -t 1"
                };
                Process.Start(proc);
            }
        }



        private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
        {

        }
    }
}
