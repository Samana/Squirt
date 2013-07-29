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
		static readonly double[] s_SupportedScales = new double[21];

		public string DocumentFileName { get; private set; }
		public TextEditor TextEditor { get { return m_TextEdit; } }

		Dictionary<int, LineMarker> m_BreakPointMarkers = new Dictionary<int, LineMarker>();
		LineMarker m_ExecutingLineMarker;

		CompletionWindow m_CompletionWindow;
		int m_CurrentScaleIndex = s_SupportedScales.Length / 2;

		static SqDocument()
		{
			try
			{
				for (int i = 0; i < s_SupportedScales.Length; i++)
				{
					s_SupportedScales[i] = Math.Pow(1.1, i - s_SupportedScales.Length / 2);
				}

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
				m_TextEdit.TextArea.PreviewMouseWheel += m_TextEdit_TextArea_PreviewMouseWheel;

				m_InnerGrid.MouseWheel += m_InnerGrid_MouseWheel;
				m_BreakPointViewer.MouseDown += m_BreakPointViewer_MouseDown;
				
				//m_BreakPointViewer.

				//FIXME : Disable auto complete for now since it's troublesome.
				//m_TextEdit.TextArea.TextEntering += TextArea_TextEntering;
				//m_TextEdit.TextArea.TextEntered += TextArea_TextEntered;

				m_TextEdit.TextArea.Caret.PositionChanged += (s, arg) =>
				{
					UpdateCursorPos();
				};

				UpdateCursorPos();
			}
			catch (IOException)
			{
			}
		}

		void UpdateCursorPos()
		{
			if (MainWindow.Instance.ActiveDocument == this)
			{
				MainWindow.Instance.SetDocumentCursorPos(m_TextEdit.TextArea.Caret.Line, m_TextEdit.TextArea.Caret.Column);
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

		private void m_TextEdit_TextArea_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
		{
			if (!e.Handled)
			{
				e.Handled = true;
				var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta);
				eventArg.RoutedEvent = UIElement.MouseWheelEvent;
				eventArg.Source = sender;
				m_InnerGrid.RaiseEvent(eventArg);
			}
		}

		void m_InnerGrid_MouseWheel(object sender, MouseWheelEventArgs e)
		{
			if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
			{
				int delta = e.Delta / 120;
				m_CurrentScaleIndex = Math.Max(0, Math.Min(s_SupportedScales.Length - 1, m_CurrentScaleIndex + delta));
				double scale = s_SupportedScales[m_CurrentScaleIndex];
				m_InnerGrid.RenderTransform = new ScaleTransform(scale, scale);
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

		void m_BreakPointViewer_MouseDown(object sender, MouseButtonEventArgs e)
		{
			var pos = e.GetPosition(m_BreakPointViewer);
			var line = m_TextEdit.TextArea.TextView.GetDocumentLineByVisualTop(pos.Y);

			if (line != null)
			{
				LineMarker breakPoint;
				if (m_BreakPointMarkers.TryGetValue(line.LineNumber, out breakPoint))
				{
					m_BreakPointMarkers.Remove(line.LineNumber);
					m_BreakPointViewer.Children.Remove(breakPoint.UIElement);
				}
				else
				{
					//FIXME: Move these to LineMarker
					double offset = 1;
					var ellipse = new Ellipse()
					{
						Width =  14,
						Height = 14,
						Fill = Brushes.Crimson,
						Stroke = Brushes.White,
						StrokeThickness = 1.2
					};
					double lineY = m_TextEdit.TextArea.TextView.GetVisualPosition(new TextViewPosition(line.LineNumber, 1), VisualYPosition.LineTop).Y + offset;
					ellipse.RenderTransform = new TranslateTransform(offset, lineY);
					m_BreakPointViewer.Children.Add(ellipse);

					m_BreakPointMarkers.Add(line.LineNumber,
						new LineMarker()
						{
							UIElement = ellipse
						});
				}
			}
		}

		internal bool HandleBreakPoint(sqnet.DebugHookType type, string funcName, int line)
		{
			LineMarker breakPoint;
			if (m_BreakPointMarkers.TryGetValue(line, out breakPoint))
			{
				Dispatcher.BeginInvoke((System.Threading.ThreadStart)delegate
				{
					if (m_ExecutingLineMarker == null)
					{
						var arrow = new Polygon()
						{
							Fill = Brushes.Yellow,
							Stroke = Brushes.DimGray,
							StrokeThickness = 0.9,
						};
						var points = new PointCollection();

						points.Add(new Point(3, 5.5));
						points.Add(new Point(9, 5.5));
						points.Add(new Point(9, 2));
						points.Add(new Point(14, 8));
						points.Add(new Point(9, 14));
						points.Add(new Point(9, 10.5));
						points.Add(new Point(3, 10.5));
						arrow.Points = points;
						m_ExecutingLineMarker = new LineMarker() { UIElement = arrow };
						m_BreakPointViewer.Children.Add(m_ExecutingLineMarker.UIElement);
						Canvas.SetZIndex(m_ExecutingLineMarker.UIElement, 10000);
					}

					double lineY = m_TextEdit.TextArea.TextView.GetVisualPosition(new TextViewPosition(line, 1), VisualYPosition.LineTop).Y;
					m_ExecutingLineMarker.UIElement.Visibility = System.Windows.Visibility.Visible;
					m_ExecutingLineMarker.UIElement.RenderTransform = new TranslateTransform(0, lineY);
				},
				System.Windows.Threading.DispatcherPriority.Send);

				return true;
			}
			return false;
		}

		internal void RemoveExecutingMarker()
		{
			if (m_ExecutingLineMarker != null && m_BreakPointViewer.Children.Contains(m_ExecutingLineMarker.UIElement))
			{
				m_ExecutingLineMarker.UIElement.Visibility = System.Windows.Visibility.Hidden;
			}
		}

		class LineMarker
		{
			public UIElement UIElement;
		}
	}
}
