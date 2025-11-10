using System;
using System.Collections.Generic;
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

        private int totalTypes = 0;

        public Inspector()
        {
            InitializeComponent();
            IsFormReady.Set();
        }

        internal void SetRefreshing(bool isRefreshing)
        {
            treeView1.Enabled = !isRefreshing;
            button1.Enabled = !isRefreshing;
            button1.Text = isRefreshing ? "Refreshing..." : "Refresh list";
            label1.Text = isRefreshing ? "Refreshing, please wait..." : $"{totalTypes} types found";
        }

        internal void PopulateTreeView(List<string> allTypes)
        {
            totalTypes = 0;

            TypeTreeEntry rootEntry = new TypeTreeEntry(0);

            allTypes.Sort();
            foreach (var type in allTypes)
            {
                if (!type.StartsWith("/"))
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
                            newEntry.Nodes.Add(fullPath + file, file);
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
            MessageBox.Show("Checking this option will hide most irrelevant types that do not use property text. " +
                "It is recommended to ALWAYS keep it checked unless you're certain that the type you're looking for is behind this option.\n\n" +
                "Toggling this option requires a refresh.",
                "OpenWF Enabler", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
    }
}
