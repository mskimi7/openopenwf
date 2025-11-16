using System;
using System.Windows.Forms;

namespace openopenclr
{
    public partial class PropertySettings : Form
    {
        // 0x0 vs 0x2 - no indentation vs indentation
        // 0x800000 vs 0x1000000 - own text (no parent props) vs full text (include parent props)
        // 0x20000000 vs 0x40000000 - relative vs absolute paths
        internal uint PropertyTextFlags { get; private set; }

        public PropertySettings(uint currentFlags)
        {
            InitializeComponent();
            PropertyTextFlags = currentFlags;
            DialogResult = DialogResult.Cancel;

            checkBox1.Checked = (currentFlags & 0x40000000u) != 0;
            checkBox2.Checked = (currentFlags & 0x1000000u) != 0;
            checkBox3.Checked = (currentFlags & 0x2u) != 0;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
            Close();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            PropertyTextFlags &= ~(0x20000000u | 0x40000000u);
            PropertyTextFlags |= checkBox1.Checked ? 0x40000000u : 0x20000000u;
        }

        private void checkBox2_CheckedChanged(object sender, EventArgs e)
        {
            PropertyTextFlags &= ~(0x800000u | 0x1000000u);
            PropertyTextFlags |= checkBox2.Checked ? 0x1000000u : 0x800000u;
        }

        private void checkBox3_CheckedChanged(object sender, EventArgs e)
        {
            PropertyTextFlags &= ~0x2u;
            PropertyTextFlags |= checkBox3.Checked ? 0x2u : 0u;
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            MessageBox.Show(this, "If checked, all types will always be fully qualified. Otherwise, types that are located in the same scope (directory) will have their scope stripped.\n\n" +
                "If unsure, leave unchecked.", "OpenWF Enabler", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void linkLabel2_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            MessageBox.Show(this, "Checking this option will include all property values, including ones that are inherited from parent type(s).\n\n" +
                "If unsure, leave unchecked.", "OpenWF Enabler", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void linkLabel3_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            MessageBox.Show(this, "Adds whitespace in the raw property text to improve readability. This option does not influence the JSON serializer.\n\n" +
                "If unsure, leave checked.", "OpenWF Enabler", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
    }
}
