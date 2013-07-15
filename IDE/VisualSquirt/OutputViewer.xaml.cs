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
	/// Interaction logic for OutputViewer.xaml
	/// </summary>
	public partial class OutputViewer : UserControl
	{
		public OutputViewer()
		{
			InitializeComponent();
		}

		private void BtnConsoleClear_Click(object sender, RoutedEventArgs e)
		{
			//TextBoxOutput.Document.Blocks.Clear();
			TextBoxOutput.Text = "";
		}

		private void TextBoxOutput_TextChanged(object sender, TextChangedEventArgs e)
		{
		}
	}
}
