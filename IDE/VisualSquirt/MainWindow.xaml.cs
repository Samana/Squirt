using Microsoft.Win32;
using sqnet;
using SqWrite.Util;
using System;
using System.Collections.Generic;
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
using Xceed.Wpf.AvalonDock.Layout;

namespace SqWrite
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		public static MainWindow Instance
		{
			get;
			private set;
		}

		SQVM m_VM;
		bool IsBuilding = false;

		public SqDocument ActiveDocument
		{
			get { return (SqDocument)GetValue(ActiveDocumentProperty); }
			private set { SetValue(ActiveDocumentProperty, value); }
		}

		public static readonly DependencyProperty ActiveDocumentProperty =
			DependencyProperty.Register("ActiveDocument", typeof(SqDocument), typeof(MainWindow), new PropertyMetadata(null));

		public MainWindow()
		{
			Instance = this;
			InitializeComponent();
			OpenDocument(null);
		}

		void OpenDocument(string fileName)
		{
			SqDocument doc = new SqDocument(fileName);
			Xceed.Wpf.AvalonDock.Layout.LayoutDocument layoutDoc = new Xceed.Wpf.AvalonDock.Layout.LayoutDocument();
			layoutDoc.Content = doc;
			layoutDoc.Closed += layoutDoc_Closed;
			m_CodeDocumentPane.InsertChildAt(0, layoutDoc);
			doc.Loaded += (s, e) =>
			{
				layoutDoc.Title = System.IO.Path.GetFileName(doc.DocumentFileName);
			};
			m_DockingManager.ActiveContent = doc;
		}

		void SaveDocument(SqDocument doc)
		{
			doc.Save();
		}

		private void ExecCommandNew(RoutedEventArgs e)
		{
		}

		private void ExecCommandOpen(RoutedEventArgs e)
		{
			OpenFileDialog ofd = new OpenFileDialog();
			ofd.Filter = "*.nut|*.nut|*.wet|*.wet";
			if (ofd.ShowDialog() ?? true)
			{
				OpenDocument(ofd.FileName);
			}
		}

		private void ExecCommandSave(RoutedEventArgs e)
		{
			if (ActiveDocument != null)
			{
				SaveDocument(ActiveDocument);
			}
		}

		private async Task ExecCommandRun(RoutedEventArgs e)
		{
			if (ActiveDocument != null)
			{
				IsBuilding = true;
				var fileName = ActiveDocument.DocumentFileName;
				await Task.Run(delegate
				{
					var bSucComp = m_VM.compilestatic(new string[] { fileName }, "test", true);
					m_VM.pushroottable();
					var bSucCall = m_VM.call(1, false, true);
				});
				IsBuilding = false;
			}
		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			//RichTextBoxWriter tbw = new RichTextBoxWriter(OutputViewer.TextBoxOutput);
			TextBoxWriter tbw = new TextBoxWriter(OutputViewer.TextBoxOutput);
			System.Console.SetError(tbw);
			System.Console.SetOut(tbw);
			Console.WriteLine("...");
			m_VM = new SQVM();
		}

		private void Window_Closed(object sender, EventArgs e)
		{
			if (m_VM != null)
			{
				m_VM.Dispose();
				m_VM = null;
			}
		}

		void layoutDoc_Closed(object sender, EventArgs e)
		{
			var doc = (sender as LayoutDocument);
			if (doc.Content != null && ActiveDocument == doc.Content)
			{
				ActiveDocument = null;
			}
		}

		private void m_DockingManager_ActiveContentChanged(object sender, EventArgs e)
		{
			var content = m_DockingManager.ActiveContent as SqDocument;
			if (content != null)
			{
				ActiveDocument = content as SqDocument;
			}
		}

		private void CommandBinding_CanExecute(object sender, CanExecuteRoutedEventArgs e)
		{
			if (e.Command == ApplicationCommands.New
				|| e.Command == ApplicationCommands.Open)
			{
				e.CanExecute = true;
			}
			else if (e.Command == ApplicationCommands.Save)
			{
				e.CanExecute = ActiveDocument != null;	//FIXME: && modified
			}
			else if (e.Command == Commands.SaveAll)
			{
				e.CanExecute = false;	//Not implemented
			}
			else if (e.Command == Commands.Run)
			{
				e.CanExecute = ActiveDocument != null && !IsBuilding;
			}
			else if (e.Command == Commands.StepThrough
				|| e.Command == Commands.StepIn
				|| e.Command == Commands.StepOut
				|| e.Command == Commands.RunToCursor)
			{
			}
		}

		private async void CommandBinding_Executed(object sender, ExecutedRoutedEventArgs e)
		{
			if (e.Command == ApplicationCommands.Save)
			{
				ExecCommandSave(e);
			}
			else if (e.Command == ApplicationCommands.Open)
			{
				ExecCommandOpen(e);
			}
			else if (e.Command == ApplicationCommands.New)
			{
				ExecCommandNew(e);
			}
			else if (e.Command == Commands.Run)
			{
				await ExecCommandRun(e);
			}
		}

		//FIXME: Ugly ugly
		internal void SetDocumentCursorPos(int line, int col)
		{
			m_StatusCursorLn.Content = string.Format("Ln {0}", line);
			m_StatusCursorCol.Content = string.Format("Col {0}", col);
		}
	}
}