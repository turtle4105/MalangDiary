using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using MalangDiary.Structs;

namespace MalangDiary.Models {
    public class UserSession {

        public Stack<string> NavigationHistory { get; set; } = new Stack<string>(); // 탐색 기록 스택, 기본값은 빈 스택

        int CurrentParentUid  = 0;   // UID of the currently logged-in parent
        int CurrentChildUid = 0;      // UID of the currently selected Child
        int CurrentDiaryUid   = 0;   // UID of the currently selected diary
        
        int CurrentDiaryIndex = 0;   // Index of the currently selected diary in the CurrentChildInfo's diary list
        int CurrentChildIndex  = 0;   // Index of the currently selected Child in the AllChildInfo array

        public void SetCurrentParentUid(int uid) { CurrentParentUid = uid; }
        public int GetCurrentParentUid() { return CurrentParentUid; }
        public void SetCurrentDiaryUid(int uid) { CurrentDiaryUid = uid; }
        public int GetCurrentDiaryUid() { return CurrentDiaryUid; }
        public void SetCurrentDiaryIndex(int index) { CurrentDiaryIndex = index; }
        public int GetCurrentDiaryIndex() { return CurrentDiaryIndex; }
        public void SetCurrentChildIndex(int index) { CurrentChildIndex = index; }
        public int GetCurrentChildIndex() { return CurrentChildIndex; }
        public void SetCurrentChildUid(int uid) { CurrentChildUid = uid; }
        public int GetCurrentChildUid() { return CurrentChildUid; }
    }
}