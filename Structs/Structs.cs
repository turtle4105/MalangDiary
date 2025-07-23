using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.Structs;

/// <summary>
/// '서버와의 소켓 통신을 위한 작업 단위.'
/// </summary>
public struct WorkItem {
    public string json;    // 보낼 JSON 문자열
    public byte[] payload; // 보낼 파일 데이터 (없으면 null 또는 빈 배열)
    public string path;    // 파일 경로 (클라이언트 개인용)
}

public struct ParentInfo {
    /*
    * parent_uid       [v]
    * id(email)        [v]
    * pw               [x]
    * nickname(name)   [v]
    * create_date      [v]
    * phone_number     [x]
    */
    public int Uid;      // parent_uid
    public string Name;  // nickname(name)
    public string Email; // id(email)
}


// Uid, Name, BirthDate, Age, Gender, IconColor
public struct ChildInfo {
    /*
    * child_uid        [v]
    * parents_id       [x]
    * name             [v]
    * birth_date       [v]
    * gender           [v]
    * voice_embedding  [x]
    * icon_color       [v]
    */
    public int Uid;             // child_uid
    public string Name;         // name
    public string BirthDate;    // birth_date
    public int Age;             // 나이 (계산된 값)
    public string Gender;       // gender
    public string IconColor;    // icon_color
}


public struct DiaryInfo {
    /*
    * diary_uid    [v]
    * child_id     [x]
    * refined_text [v]
    * photo_path   [x]
    * create_at    [v]
    * is_liked     [v]
    * weather      [v]
    * title        [v]
    * is_deleted   [x]
    */
    public int Uid;              // diary_uid
    public string Title;         // title
    public string Text;          // refined_text
    public int IntWeather;       // weather
    public string Weather;       // weather (문자열로 변환된 값)
    public bool IsLiked;         // is_liked
    public string PhotoFileName; // photo_path
    public string Date;          // create_at
    public List<string> Emotions;    // 감정 태그 배열
}
