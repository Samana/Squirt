using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace SqWrite.Util
{
	class TextBoxWriter : TextWriter
	{
		TextBox m_Output;

		public TextBoxWriter(TextBox output)
		{
			m_Output = output;
		}

		public override void Write(char value)
		{
			Action action = delegate { m_Output.AppendText(value.ToString()); };
			m_Output.Dispatcher.BeginInvoke(action);
		}

		public override Encoding Encoding
		{
			get { return System.Text.Encoding.UTF8; }
		}
	}
}
