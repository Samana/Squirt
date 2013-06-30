using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace SqWrite.Util
{
	class TextBoxWriter : TextWriter
	{
		RichTextBox m_Output;

		public TextBoxWriter(RichTextBox output)
		{
			m_Output = output;
		}

		public override void Write(char value)
		{
			Color color = sqnet.Cons.ForegroundColor;
			Action action = delegate
			{
				AppendText(m_Output, new SolidColorBrush(color), value.ToString());
			};
			m_Output.Dispatcher.BeginInvoke(action);
		}

		public override void Write(char[] buffer, int index, int count)
		{
			Color color = sqnet.Cons.ForegroundColor;
			Action action = delegate
			{
				AppendText(m_Output, new SolidColorBrush(color), new string(buffer).Replace('\n', '\r'));	//But why...
			};
			m_Output.Dispatcher.BeginInvoke(action);
		}

		public override Encoding Encoding
		{
			get { return System.Text.Encoding.UTF8; }
		}

		void AppendText(RichTextBox box, Brush color, string text)
		{
			TextPointer end = box.Document.ContentEnd;
			TextRange tr = new TextRange(end, end);
			tr.Text = text;
			tr.ApplyPropertyValue(TextElement.ForegroundProperty, color);
		}
	}
}
