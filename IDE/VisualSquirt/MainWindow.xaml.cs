using Microsoft.Win32;
using sqnet;
using SqWrite.Util;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
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

		object m_sync = new object();

		SQVM m_VM;
		volatile bool m_IsBuilding = false;
		volatile bool m_IsPaused = false;

		public bool IsBuilding
		{
			get { lock (m_sync) { return m_IsBuilding; } }
			set { lock (m_sync) { m_IsBuilding = value; } }
		}

		public bool IsPaused
		{
			get { lock (m_sync) { return m_IsPaused; } }
			set { lock (m_sync) { m_IsPaused = value; } }
		}

		System.Threading.AutoResetEvent m_BreakPointLock = new System.Threading.AutoResetEvent(false);

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
			UpdateLayout();
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
				if (IsPaused)
				{
					ActiveDocument.RemoveExecutingMarker();
					IsPaused = false;
					m_BreakPointLock.Set();
				}
				else
				{
					IsBuilding = true;
					UpdateLayout();

					var fileName = ActiveDocument.DocumentFileName;
					await Task.Run(delegate
					{
						var bSucComp = m_VM.compilestatic(new string[] { fileName }, "test", true);
						m_VM.pushroottable();
						var bSucCall = m_VM.call(1, false, true);
					});
					IsBuilding = false;
				}
				UpdateLayout();
			}
		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			//RichTextBoxWriter tbw = new RichTextBoxWriter(OutputViewer.TextBoxOutput);
			TextBoxWriter tbw = new TextBoxWriter(OutputViewer.TextBoxOutput);
			System.Console.SetError(tbw);
			System.Console.SetOut(tbw);
			m_VM = new SQVM();
			//FIXME: Change to binding
			//FIXME: Disable "Jit_llvm" when it's not defined.
			m_VM.DebugInfoEnabled = Cmb_DebugType.SelectedIndex == 0 ? true : false;
			m_VM.RuntimeType = Cmb_RuntimeType.SelectedIndex == 0 ? ERuntimeType.Jit_llvm : ERuntimeType.Interpreter;
			m_VM.OnDebugCallback += m_VM_OnDebugCallback;
		}

		void m_VM_OnDebugCallback(DebugHookType type, string sourceName, string funcName, int line)
		{
			//FIXME: Only checking ActiveDocument since we only compile/run that single file for nows.
			SqDocument activeDocument = null;
			Dispatcher.BeginInvoke(
				(ThreadStart)delegate
				{
					activeDocument = ActiveDocument;
				},
				System.Windows.Threading.DispatcherPriority.Send).Wait();

			if (activeDocument != null && activeDocument.DocumentFileName == sourceName)
			{
				if (activeDocument.HandleBreakPoint(type, funcName, line))
				{
					Dispatcher.BeginInvoke(
						(ThreadStart)delegate
						{
							OnScriptThreadPause();
						},
						System.Windows.Threading.DispatcherPriority.Send);
					m_BreakPointLock.WaitOne();
					//Dispatcher.BeginInvoke();
				}
			}
		}

		void OnScriptThreadPause()
		{
			m_BreakPointLock.Reset();
			IsPaused = true;
			UpdateLayout();
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
				e.CanExecute = ActiveDocument != null && ( (!IsBuilding) || IsPaused);
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

		private void Cmb_DebugType_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			if (m_VM != null)
			{
				m_VM.DebugInfoEnabled = (sender as ComboBox).SelectedIndex == 0 ? true : false;
			}
		}

		private void Cmb_RuntimeType_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			if (m_VM != null)
			{
				m_VM.RuntimeType = (ERuntimeType)(sender as ComboBox).SelectedIndex;
			}
		}
	}
}