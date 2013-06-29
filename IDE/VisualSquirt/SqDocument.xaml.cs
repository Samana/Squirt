using ICSharpCode.AvalonEdit;
using ICSharpCode.AvalonEdit.CodeCompletion;
using ICSharpCode.AvalonEdit.Document;
using ICSharpCode.AvalonEdit.Editing;
using ICSharpCode.AvalonEdit.Highlighting;
using ICSharpCode.AvalonEdit.Rendering;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Xml;
using System.Xml.Linq;

namespace SqWrite
{
	/// <summary>
	/// Interaction logic for SqDocument.xaml
	/// </summary>
	public partial class SqDocument : UserControl
	{
		static readonly string s_TempFolder;
		static readonly string[] s_Keywords;

		public string DocumentFileName { get; private set; }

		CompletionWindow m_CompletionWindow;

		static SqDocument()
		{
			try
			{
				IHighlightingDefinition customHighlighting;
				using (Stream s = typeof(SqDocument).Assembly.GetManifestResourceStream("SqWrite.Squirrel.xshd"))
				{
					if (s == null)
						throw new InvalidOperationException("Could not find embedded resource");
					using (XmlReader reader = new XmlTextReader(s))
					{
						customHighlighting = ICSharpCode.AvalonEdit.Highlighting.Xshd.
							HighlightingLoader.Load(reader, HighlightingManager.Instance);
					}
				}

				using (Stream s = typeof(SqDocument).Assembly.GetManifestResourceStream("SqWrite.Squirrel.xshd"))
				{
					XDocument xdoc = XDocument.Load(s);
					s_Keywords = (from child in xdoc.Root.Descendants() where child.Name.LocalName == "Word" select child.Value).ToArray();
				}
				HighlightingManager.Instance.RegisterHighlighting("Squirrel", new string[] { ".nut", ".wet" }, customHighlighting);
			}
			catch
			{
			}

			try
			{
				s_TempFolder = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "TempDocs");
				if (Directory.Exists(s_TempFolder))
				{
					CleanUpAllTempFiles();
				}
				else
				{
					Directory.CreateDirectory(s_TempFolder);
				}
			}
			catch
			{
			}
		}

		public SqDocument()
		{
			InitializeComponent();
		}

		public SqDocument(string fileName)
		{
			InitializeComponent();
			DocumentFileName = fileName;
		}

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			try
			{
				//FIXME: Delete all temp files when application exits.
				if (DocumentFileName == null)
				{
					string tempFolder = System.IO.Path.GetTempPath();
					int i = 0;
					for (; ; )
					{
						DocumentFileName = System.IO.Path.Combine(s_TempFolder, string.Format("new {0}.nut", i));
						if (File.Exists(DocumentFileName))
						{
							i++;
							continue;
						}
						using (File.Create(DocumentFileName)) { }
						break;
					}
				}
				m_TextEdit.Load(DocumentFileName);
				m_TextEdit.TextArea.MouseWheel += m_TextEdit_TextArea_MouseWheel;

				//FIXME : Disable auto complete for now since it's troublesome.
				//m_TextEdit.TextArea.TextEntering += TextArea_TextEntering;
				//m_TextEdit.TextArea.TextEntered += TextArea_TextEntered;
			}
			catch (IOException)
			{
			}
		}

		void TextArea_TextEntering(object sender, TextCompositionEventArgs e)
		{
			if (e.Text.Length > 0 && m_CompletionWindow != null)
			{
				if (!char.IsLetterOrDigit(e.Text[0]))
				{
					m_CompletionWindow.CompletionList.RequestInsertion(e);
				}
			}
		}

		void TextArea_TextEntered(object sender, TextCompositionEventArgs e)
		{
			if (e.Text == ".")
			{
				// Open code completion after the user has pressed dot:
				//m_CompletionWindow = new CompletionWindow(m_TextEdit.TextArea);
				//IList<ICompletionData> data = m_CompletionWindow.CompletionList.CompletionData;
				//data.Add(new MyCompletionData("Item1"));
				//m_CompletionWindow.Show();
				//m_CompletionWindow.Closed += delegate
				//{
				//	m_CompletionWindow = null;
				//};
			}
			else
			{
				if (m_CompletionWindow == null)
				{
					m_CompletionWindow = new CompletionWindow(m_TextEdit.TextArea);
					IList<ICompletionData> data = m_CompletionWindow.CompletionList.CompletionData;
					bool bAddAll = string.IsNullOrWhiteSpace(e.Text);
					foreach (var kw in s_Keywords)
					{
						if (kw.StartsWith(e.Text) || bAddAll)
						{
							data.Add(new TextCompletionData(kw, m_TextEdit));
						}
					}
				} 
				m_CompletionWindow.Show();
				m_CompletionWindow.Closed += delegate
				{
					m_CompletionWindow = null;
				};
			}
		}

		static void CleanUpAllTempFiles()
		{
			try
			{
				string[] files = Directory.GetFiles(s_TempFolder, "new *.nut");
				foreach (var file in files)
				{
					File.Delete(file);
				}
			}
			catch
			{
			}
		}

		private void m_TextEdit_TextArea_MouseWheel(object sender, MouseWheelEventArgs e)
		{
			if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
			{
				double textSize = m_TextEdit.FontSize;
				textSize += 2 * e.Delta / 120;
				textSize = Math.Max(5, Math.Min(120, textSize));
				m_TextEdit.FontSize = textSize;
				e.Handled = true;
			}
		}

		class TextCompletionData : ICompletionData
		{
			public TextCompletionData(string text, TextEditor ed)
			{
				this.Text = text;
				m_Ed = ed;
			}

			public System.Windows.Media.ImageSource Image
			{
				get { return null; }
			}

			TextEditor m_Ed;
			public string Text { get; private set; }

			// Use this property if you want to show a fancy UIElement in the list.
			public object Content
			{
				get { return this.Text; }
			}

			public object Description
			{
				get { return "Description for " + this.Text; }
			}

			public void Complete(TextArea textArea, ISegment completionSegment,
				EventArgs insertionRequestEventArgs)
			{
				textArea.Document.Replace(completionSegment, this.Text);
			}

			public double Priority
			{
				get { return 0; }
			}
		}

		public void Save()
		{
			m_TextEdit.Save(DocumentFileName);
		}

		public void Compile()
		{
		}
	}
}
