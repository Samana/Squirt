using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace SqWrite.Util
{
	public static class Commands
	{
		public static readonly RoutedUICommand SaveAll = new RoutedUICommand("Save All", "SaveAll", typeof(MainWindow));
		public static readonly RoutedUICommand Run = new RoutedUICommand("Run", "Run", typeof(MainWindow));
		public static readonly RoutedUICommand StepThrough = new RoutedUICommand("Step Through", "StepThrough", typeof(MainWindow));
		public static readonly RoutedUICommand StepIn = new RoutedUICommand("Step In", "StepIn", typeof(MainWindow));
		public static readonly RoutedUICommand StepOut = new RoutedUICommand("Step Out", "StepOut", typeof(MainWindow));
		public static readonly RoutedUICommand RunToCursor = new RoutedUICommand("Run To Cursor", "RunToCursor", typeof(MainWindow));
	}
}