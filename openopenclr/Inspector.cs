using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Threading;
using System.Windows.Forms;
using openopenclr.NativeEvents;

namespace openopenclr
{
    public partial class Inspector : Form
    {
        private class TypeTreeEntry
        {
            internal Dictionary<string, TypeTreeEntry> directories;
            internal HashSet<string> files;
            internal int nestingLevel;

            internal void AddFile(string name)
            {
                if (files == null)
                    files = new HashSet<string>();

                files.Add(name);
            }

            internal TypeTreeEntry GetOrAddDirectory(string name)
            {
                if (directories == null)
                    directories = new Dictionary<string, TypeTreeEntry>();

                if (!directories.TryGetValue(name, out TypeTreeEntry subdir))
                {
                    subdir = new TypeTreeEntry(nestingLevel + 1);
                    directories.Add(name, subdir);
                }

                return subdir;
            }

            internal TypeTreeEntry(int nestingLevel)
            {
                this.nestingLevel = nestingLevel;

                if (nestingLevel <= 2)
                {
                    directories = new Dictionary<string, TypeTreeEntry>();
                    files = new HashSet<string>();
                }
            }
        }

        internal EventWaitHandle IsFormReady { get; } = new EventWaitHandle(false, EventResetMode.ManualReset);

        private readonly Dictionary<string, TreeNode> allFileNodes = new Dictionary<string, TreeNode>();
        private readonly List<LinkLabel> extraInheritanceControls = new List<LinkLabel>();
        private int totalTypes = 0;

        private PropertyText currentPropertyText;
        private Dictionary<uint, byte[]> availablePropertyTexts = new Dictionary<uint, byte[]>();
        private uint requestedPropertyFlags = 0x20800003; // relative, own text, with indentation

        public Inspector()
        {
            InitializeComponent();
            IsFormReady.Set();
        }

        private void OnInheritanceLinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            LinkLabel label = (LinkLabel)sender;
            if (!allFileNodes.TryGetValue(label.Text, out TreeNode node))
            {
                MessageBox.Show("Could not navigate to the selected type (not found)", "OpenWF Enabler", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            treeView1.SelectedNode = node;
            treeView1.Focus();
        }

        internal void SetRefreshing(bool isRefreshing)
        {
            treeView1.Enabled = !isRefreshing;
            button1.Enabled = !isRefreshing;
            button1.Text = isRefreshing ? "Refreshing..." : "Refresh list";
            label1.Text = isRefreshing ? "Refreshing, please wait..." : $"{totalTypes} types found";
        }

        internal void SetPropertyText(uint flags)
        {
            if (availablePropertyTexts == null || !availablePropertyTexts.TryGetValue(flags, out byte[] propText))
            {
                currentPropertyText = null;
                textBox1.ForeColor = Color.DarkRed;
                textBox1.Text = "No property text is currently available.";
            }
            else
            {
                try
                {
                    currentPropertyText = new PropertyText(propText);
                }
                catch (Exception ex)
                {
                    currentPropertyText = null;
                    textBox1.ForeColor = Color.DarkRed;
                    textBox1.Text = $"Property text is invalid:\r\n{ex}";
                    return;
                }

                textBox1.ForeColor = Color.Black;
                textBox1.Text = radioButton1.Checked ? currentPropertyText.TextData : currentPropertyText.JsonData;
            }
        }

        internal void SetAvailablePropertyTexts(Dictionary<uint, byte[]> availableTexts)
        {
            availablePropertyTexts = availableTexts;
            SetPropertyText(requestedPropertyFlags);
        }

        internal void SetInheritanceInfo(List<string> inheritanceChain)
        {
            if (inheritanceChain == null || inheritanceChain.Count == 0)
            {
                foreach (var linkLabel in extraInheritanceControls)
                {
                    linkLabel.Dispose();
                    tabPage1.Controls.Remove(linkLabel);
                }

                extraInheritanceControls.Clear();

                label3.Visible = false;
                label4.Visible = false;
                linkLabel2.Visible = false;
            }
            else
            {
                inheritanceChain = inheritanceChain.Reverse<string>().ToList(); // make a copy
                linkLabel2.Text = inheritanceChain[0];

                LinkLabel prevLinkLabel = linkLabel2;
                foreach (var type in inheritanceChain.Skip(1))
                {
                    LinkLabel ll = new LinkLabel
                    {
                        AutoSize = true,
                        Visible = true,
                        TabIndex = prevLinkLabel.TabIndex + 1,
                        TabStop = true,
                        Text = type,
                        Location = new Point(prevLinkLabel.Location.X + 8, prevLinkLabel.Location.Y + 16)
                    };

                    tabPage1.Controls.Add(ll);
                    extraInheritanceControls.Add(ll);

                    ll.LinkClicked += OnInheritanceLinkClicked;

                    prevLinkLabel = ll;
                }

                label3.Visible = true;
                label4.Visible = true;
                linkLabel2.Visible = true;
            }

            tabPage1.Refresh();
        }

        internal void PopulateTreeView(List<string> allTypes)
        {
            totalTypes = 0;

            TypeTreeEntry rootEntry = new TypeTreeEntry(0);

            allTypes.Sort();
            foreach (var type in allTypes)
            {
                if (type.Length == 0 || type[0] != '/')
                {
                    NativeInterface.LogToConsole($"Ignored abnormal type: {type}");
                    continue;
                }

                TypeTreeEntry currEntry = rootEntry;
                int lastComponentStartIdx = 1;
                for (; ;)
                {
                    int slashIdx = type.IndexOf('/', lastComponentStartIdx);
                    if (slashIdx == -1) // file
                    {
                        currEntry.AddFile(type.Substring(lastComponentStartIdx));
                        break;
                    }
                    else // directory
                    {
                        currEntry = currEntry.GetOrAddDirectory(type.Substring(lastComponentStartIdx, slashIdx - lastComponentStartIdx));
                        lastComponentStartIdx = slashIdx + 1;
                    }
                }
            }

            void InsertEntries(TreeNodeCollection parentEntry, string parentName, string name, TypeTreeEntry children)
            {
                string fullPath = parentName + name;
                TreeNode newEntry = parentEntry.Add(fullPath, name);

                if (children != null)
                {
                    if (children.directories != null)
                    {
                        foreach (var subdir in children.directories)
                        {
                            InsertEntries(newEntry.Nodes, fullPath, subdir.Key + "/", subdir.Value);
                        }
                    }

                    if (children.files != null)
                    {
                        foreach (var file in children.files)
                        {
                            ++totalTypes;

                            string fullName = fullPath + file;
                            allFileNodes[fullName] = newEntry.Nodes.Add(fullName, file);
                        }
                    }
                }
            }

            InsertEntries(treeView1.Nodes, "", "/", rootEntry);
        }

        internal void OnTypeListReceived(ResponseTypeListEvent evt)
        {
            BeginInvoke((Action)(() =>
            {
                treeView1.BeginUpdate();
                try
                {
                    // suppressing WM_NOTIFY improves treeview clearing performance because:
                    // - each element is deleted separately (internally, in the native comctl32.dll)
                    // - each element's deletion triggers WM_NOTIFY that's sent using a syscall
                    // - so we just prevent the syscall from being sent which improves performance by like x100
                    // - we don't care about WM_NOTIFY anyway
                    NativeInterface.SuppressTreeNodeEvents(true);
                    treeView1.Nodes.Clear();
                    allFileNodes.Clear();
                    NativeInterface.SuppressTreeNodeEvents(false);

                    PopulateTreeView(evt.AllTypes);
                    if (treeView1.Nodes.Count > 0)
                        treeView1.Nodes[0].Expand();
                }
                finally
                {
                    treeView1.EndUpdate();
                    SetRefreshing(false);
                }
            }));
        }

        internal void OnTypeInfoReceived(ResponseTypeInfoEvent evt)
        {
            BeginInvoke((Action)(() =>
            {
                label2.ResetText();
                SetAvailablePropertyTexts(null);
                SetInheritanceInfo(null);

                if (evt.IsError)
                {
                    label2.Text = evt.ErrorMessage;
                }
                else
                {
                    label2.Text = $"Type info for {evt.InheritanceChain[0]}";
                    SetAvailablePropertyTexts(evt.PropertyTexts);
                    SetInheritanceInfo(evt.InheritanceChain);
                }
            }));
        }

        private void button1_Click(object sender, EventArgs e)
        {
            NativeInterface.RequestTypeList(!checkBox1.Checked);
            SetRefreshing(true);
        }

        private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
        {
            if (!e.Node.Name.EndsWith("/"))
                NativeInterface.RequestTypeInfo(e.Node.Name);
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            MessageBox.Show(this, "Checking this option will hide most irrelevant types that do not use property text. " +
                "It is recommended to ALWAYS keep it checked unless you're certain that the type you're looking for is behind this option.\n\n" +
                "Toggling this option requires a refresh.",
                "OpenWF Enabler", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void Inspector_Shown(object sender, EventArgs e)
        {
            BeginInvoke((Action)(() =>
            {
                button1.PerformClick(); // simulate type list refresh
            }));
        }

        private void Inspector_FormClosed(object sender, FormClosedEventArgs e)
        {
            NativeInterface.SuppressTreeNodeEvents(true); // improves performance of treeview clearing upon form close
        }

        private void linkLabel3_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            using (var settings = new PropertySettings(requestedPropertyFlags))
            {
                if (settings.ShowDialog(this) == DialogResult.OK)
                {
                    requestedPropertyFlags = settings.PropertyTextFlags;
                    SetPropertyText(requestedPropertyFlags);
                }
            }
        }

        private void radioButton1_CheckedChanged(object sender, EventArgs e)
        {
            SetPropertyText(requestedPropertyFlags);
        }

        private void radioButton2_CheckedChanged(object sender, EventArgs e)
        {
            SetPropertyText(requestedPropertyFlags);
        }
    }
}
