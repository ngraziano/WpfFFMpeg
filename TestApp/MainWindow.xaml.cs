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
using FfmpegProxy;

namespace TestApp
{
    /// <summary>
    /// Logique d'interaction pour MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        FFMPEGProxy proxy;

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (proxy != null)
                proxy.Dispose();
            proxy = new FFMPEGProxy();
            proxy.NewFrame += proxy_NewFrame;
            //            string uri = "h:\\Prometheus.avi";
            string uri = "rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=201&flavour=ld";
            Task.Run(
                () =>
                {
                    try
                    {
                        proxy.Open(uri);

                    }
                    catch (Exception error)
                    {
                        Console.WriteLine(error.Message);
                    }
                });


        }

        public WriteableBitmap bitmapFrame;

        void proxy_NewFrame(object sender, NewFrameEventArgs e)
        {
            Dispatcher.Invoke(new Action
            (() =>
            {
            var frame = e.NewFrame;
            if (!IsBitmapValid(frame))
            {
                bitmapFrame = new WriteableBitmap(
                    frame.Width,
                    frame.Height, 96, 96 * 16 / 9, PixelFormats.Pbgra32
                    , null
                    );
                   videodest.Source = bitmapFrame;
            }

            bitmapFrame.Lock();
            frame.CopyToBuffer(bitmapFrame.BackBuffer, bitmapFrame.BackBufferStride);
            bitmapFrame.AddDirtyRect(new Int32Rect(0, 0, frame.Width, frame.Height));
            bitmapFrame.Unlock();
            }));
            //  e.NewFrame
        }

        private bool IsBitmapValid(FfmpegProxy.Frame frame)
        {
            if (bitmapFrame == null)
                return false;

            if (bitmapFrame.Width != frame.Width)
                return false;

            // Sinon recreation systématique
            if (bitmapFrame.Height >= frame.Height)
                return false;

            return true;
        }




    }
}
