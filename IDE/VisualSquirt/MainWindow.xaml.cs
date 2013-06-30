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

namespace SqWrite
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		SQVM m_VM;

		private SqDocument ActiveDocument
		{
			get
			{
				var content = m_DockingManager.ActiveContent as Xceed.Wpf.AvalonDock.Layout.LayoutDocument;
				if (content != null)
				{
					return content.Content as SqDocument;
				}
				return null;
			}
		}

		public MainWindow()
		{
			InitializeComponent();
			OpenDocument(null);
		}

		void OpenDocument(string fileName)
		{
			SqDocument doc = new SqDocument(fileName);
			Xceed.Wpf.AvalonDock.Layout.LayoutDocument layoutDoc = new Xceed.Wpf.AvalonDock.Layout.LayoutDocument();
			layoutDoc.Content = doc;
			m_CodeDocumentPane.InsertChildAt(0, layoutDoc);
			doc.Loaded += (s, e) =>
			{
				layoutDoc.Title = System.IO.Path.GetFileName(doc.DocumentFileName);
			};
			m_DockingManager.ActiveContent = layoutDoc;
		}

		void SaveDocument(SqDocument doc)
		{
			doc.Save();
		}

		private void BtnNew_Click(object sender, RoutedEventArgs e)
		{
		}

		private void BtnOpen_Click(object sender, RoutedEventArgs e)
		{
			OpenFileDialog ofd = new OpenFileDialog();
			ofd.Filter = "*.nut|*.nut|*.wet|*.wet";
			if (ofd.ShowDialog() ?? true)
			{
				OpenDocument(ofd.FileName);
			}
		}

		private void BtnSave_Click(object sender, RoutedEventArgs e)
		{
			if (ActiveDocument != null)
			{
				SaveDocument(ActiveDocument);
			}
		}

		private void BtnSaveAll_Click(object sender, RoutedEventArgs e)
		{
		}

		private void BtnUndo_Click(object sender, RoutedEventArgs e)
		{
		}

		private void BtnRedo_Click(object sender, RoutedEventArgs e)
		{
		}

		private async void BtnGo_Click(object sender, RoutedEventArgs e)
		{
			if (ActiveDocument != null)
			{
				BtnGo.IsEnabled = false;
				var fileName = ActiveDocument.DocumentFileName;
				await Task.Run(delegate
				{
					var bSucComp = m_VM.compilestatic(new string[] { fileName }, "test", true);
					m_VM.pushroottable();
					var bSucCall = m_VM.call(1, false, true);
				});
				BtnGo.IsEnabled = true;
			}
		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			TextBoxWriter tbw = new TextBoxWriter(TextBoxOutput);
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

		private void BtnConsoleClear_Click(object sender, RoutedEventArgs e)
		{
			TextBoxOutput.Document.Blocks.Clear();
		}
	}
}