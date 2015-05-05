using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using FfmpegProxy;
using Xunit;

namespace FFMPEGProxyTest
{
    public class FFMPEGProxyTests
    {
        [Fact]
        void PlayNull()
        {
            FFMPEGProxy testProxy = new FFMPEGProxy();

            Assert.ThrowsAny<ArgumentNullException>(() => testProxy.Open(null));
        }

        [Fact]
        void PlaySomethingInvalid()
        {
            FFMPEGProxy testProxy = new FFMPEGProxy();
            // file:///
            testProxy.Open("h:\\Prometheus.avi");
        }
    }
}
