using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.Messages
{
    public class VoiceRecordedMessage
    {
        public bool IsRecorded { get; }

        public VoiceRecordedMessage(bool isRecorded)
        {
            IsRecorded = isRecorded;
        }
    }
}
