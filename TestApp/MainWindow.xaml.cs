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
            {
                var oldproxy = proxy;
                oldproxy.NewFrame -= proxy_NewFrame;
                // desynchronisation pour eviter les interblocage avec l'affichage.
                Task.Run(() => oldproxy.Dispose());
            }
            proxy = new FFMPEGProxy();
            proxy.NewFrame += proxy_NewFrame;
            string uri = "rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=201&flavour=sd";
            
            
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
            FFMPEGProxy proxy = sender as FFMPEGProxy;
            var frame = e.NewFrame;

            if (proxy == null || frame == null)
                return;

            Dispatcher.Invoke(new Action
            (() =>
            {
                if (!IsBitmapValid(frame))
                {
                    double ratio = proxy.GuessAspectRatio(frame);
                    if (ratio == 0)
                        ratio = 16 / 9;
                    bitmapFrame = new WriteableBitmap(
                        frame.Width,
                        frame.Height, 96, 96 * ratio, PixelFormats.Pbgra32
                        , null
                        );
                    videodest.Source = bitmapFrame;
                }

                bitmapFrame.Lock();
                proxy.CopyToBuffer(frame,bitmapFrame.BackBuffer, bitmapFrame.BackBufferStride);
                bitmapFrame.AddDirtyRect(new Int32Rect(0, 0, frame.Width, frame.Height));
                bitmapFrame.Unlock();
            }));
            //  e.NewFrame
        }

        private bool IsBitmapValid(FfmpegProxy.Frame frame)
        {
            if (bitmapFrame == null)
                return false;

            if (bitmapFrame.PixelWidth != frame.Width)
                return false;

            if (bitmapFrame.PixelHeight != frame.Height)
            {
                string taille = string.Format("Taille bitmap {0}, taille frame {1}", bitmapFrame.PixelHeight, frame.Height);
                Console.WriteLine(taille);
                return false;
            }
            return true;
        }




    }
}
