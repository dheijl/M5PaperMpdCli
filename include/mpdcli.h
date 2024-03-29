// Copyright (c) 2023 @dheijl (danny.heijl@telenet.be)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <cctype>
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

#include <WiFi.h>
#include <WiFiClient.h>

#include "epdfunctions.h"

using std::string;
using std::vector;

static const constexpr char* MPD_CURRENTSONG = "currentsong\n";
static const constexpr char* MPD_STATUS = "status\n";
static const constexpr char* MPD_START = "play\n";
static const constexpr char* MPD_STOP = "stop\n";
static const constexpr char* MPD_CLEAR = "clear\n";
static const constexpr char* MPD_ADD = "add {}\n";

enum MpdResponseType {
    MpdOKType,
    MpdACKType,
    MpdErrorType,
};

enum MpdResponseKind {
    MpdUninitializedKind,
    MpdConnectKind,
    MpdCurrentSongKind,
    MpdStatusKind,
    MpdCommandType,
    MpdFailureKind,
};

class MpdResponse {
private:
    MpdResponseType ResponseType;
    string Response;

protected:
    MpdResponseKind ResponseKind;
    std::map<string, string> ResponseData;
    string getItem(const string& item)
    {
        auto it = this->ResponseData.find(item);
        if (it != this->ResponseData.end()) {
            return it->second;
        } else {
            return "";
        }
    }
    virtual void abstract() = 0;

public:
    MpdResponse(const string& response)
    {
        this->ResponseKind = MpdUninitializedKind;
        this->Response = response;
        auto p = response.find("ACK");
        if (p != string::npos) {
            if (p == 0) {
                this->ResponseType = MpdACKType;
            }
        } else {
            p = response.find("OK");
            if (p != string::npos) {
                if ((p == (response.length() - 3)) || (p == 0)) {
                    this->ResponseType = MpdOKType;
                }
            } else {
                this->ResponseType = MpdErrorType;
            }
        }
        // parse response into vector and then into map
        if (this->ResponseType == MpdOKType) {
            // build a vector of the response lines
            static const string delimiter = "\n";
            size_t pos_start = 0, pos_end, delim_len = delimiter.length();
            string token;
            vector<string> lines;
            while ((pos_end = this->Response.find(delimiter, pos_start)) != string::npos) {
                token = this->Response.substr(pos_start, pos_end - pos_start);
                pos_start = pos_end + delim_len;
                lines.push_back(token);
            }
            lines.push_back(this->Response.substr(pos_start));
            // now extract key-value pairs into the map
            for (auto line : lines) {
                if ((pos_start = line.find(": ")) != string::npos) {
                    auto key = line.substr(0, pos_start);
                    for (int i = 0; i < key.length(); ++i) {
                        key[i] = std::toupper(key[i]);
                    }
                    auto value = line.substr(pos_start + 2);
                    this->ResponseData[key] = value;
                }
            }
        }
    }
    string getResponseString()
    {
        return this->Response;
    }
    MpdResponseType getResponseType()
    {
        return this->ResponseType;
    }
    MpdResponseKind getResponseKind()
    {
        return this->ResponseKind;
    }
};

class MpdConnect : public MpdResponse {
private:
    string Version;
    void abstract() override { }

public:
    MpdConnect(const string& response)
        : MpdResponse(response)
    {
        this->ResponseKind = MpdConnectKind;
        this->Version = this->getResponseString();
        int p = this->Version.find('\n');
        if (p > 0) {
            this->Version.erase(p, 1);
        }
    }
    string getVersion()
    {
        return this->Version;
    }
};

class MpdCurrentSong : public MpdResponse {
private:
    void abstract() override { }

public:
    MpdCurrentSong(const string& response)
        : MpdResponse(response)
    {
        if (this->getResponseType() == MpdOKType) {
            this->ResponseKind = MpdCurrentSongKind;
        } else {
            this->ResponseKind = MpdFailureKind;
        }
    }
    string getFile()
    {
        return this->getItem("FILE");
    }
    string getName()
    {
        return this->getItem("NAME");
    }
    string getTitle()
    {
        return this->getItem("TITLE");
    }
    string getArtist()
    {
        string artist = this->getItem("ARTIST");
        if (artist.empty()) {
            artist = this->getItem("ALBUMARTIST");
        }
        return artist;
    }
    string getAlbum()
    {
        return this->getItem("ALBUM");
    }
};

class MpdStatus : public MpdResponse {
private:
    void abstract() override { }

public:
    MpdStatus(const string& response)
        : MpdResponse(response)
    {
        if (this->getResponseType() == MpdOKType) {
            this->ResponseKind = MpdStatusKind;
        } else {
            this->ResponseKind = MpdFailureKind;
        }
    }
    string getState()
    {
        return this->getItem("STATE");
    }
    string getElapsed()
    {
        return this->getItem("ELAPSED");
    }
    string getDuration()
    {
        return this->getItem("DURATION");
    }
    string getFormat()
    {
        return this->getItem("AUDIO");
    }
    string getError()
    {
        return this->getItem("ERROR");
    }
};

class MpdSimpleCommand : public MpdResponse {
private:
    void abstract() override { }

public:
    MpdSimpleCommand(const string& response)
        : MpdResponse(response)
    {
        if (this->getResponseType() == MpdOKType) {
            this->ResponseKind = MpdCommandType;
        } else {
            this->ResponseKind = MpdFailureKind;
        }
    }
    string GetResult()
    {
        return this->getResponseType() == MpdOKType ? "OK" : "ERROR ";
    }
};

class MpdConnection {
private:
    WiFiClient Client;
    StatusLines status;
    string last_error;
    string read_data()
    {
        int n = 5000;
        while (n-- > 0) {
            vTaskDelay(1);
            if (Client.available()) {
                break;
            }
        }
        if (n <= 0) {
            this->status.push_back("no response");
            return string();
        }
        uint8_t buf[4096];
        n = Client.read(buf, sizeof(buf));
        if (n > 0) {
            string data((char*)&buf[0], n);
            return data;
        } else {
            return string("");
        }
    }

protected:
public:
    StatusLines& GetResponse()
    {
        return this->status;
    }

    string GetLastError()
    {
        return this->last_error;
    }

    bool Connect(const char* host, int port)
    {
        this->status.clear();
        this->last_error.clear();
        if (Client.connect(host, port)) {
            this->status.push_back("MPD @" + String(host) + ":" + String(port));
            string data = read_data();
            if (data.length() == 0) {
                return false;
            }
            MpdConnect con(data);
            String v = String(con.getVersion().c_str());
            if (v.indexOf("OK") < 0) {
                this->status.push_back("MPD Version: " + v);
                return false;
            }
            return true;
        } else {
            this->status.push_back("MPD Connection failed");
            return false;
        }
    }
    void Disconnect()
    {
        this->status.clear();
        // this->status.push_back("Disconnect MPD");
        Client.stop();
    }

    bool GetStatus()
    {
        this->status.clear();
        Client.write(MPD_STATUS);
        string data = read_data();
        if (data.length() == 0) {
            return false;
        }
        this->status.push_back(" ");
        MpdStatus mpd_status(data);
        auto mpdstatus = mpd_status.getState();
        auto format = mpd_status.getFormat();
        if (mpdstatus == "play") {
            this->status.push_back("Playing (" + String(format.c_str()) + ")");
        } else {
            this->status.push_back("Stopped (" + String(format.c_str()) + ")");
        }
        this->last_error = mpd_status.getError();
        if (!this->last_error.empty()) {
            this->status.push_back("*ERR: " + String(this->last_error.c_str()));
        }
        return mpdstatus.compare("play") == 0;
    }

    bool IsPlaying()
    {
        this->status.clear();
        Client.write(MPD_STATUS);
        string data = read_data();
        if (data.length() == 0) {
            return false;
        }
        MpdStatus mpd_status(data);
        auto mpdstatus = mpd_status.getState();
        this->status.push_back("MPD status: " + String(mpdstatus.c_str()));
        return mpdstatus.compare("play") == 0;
    }

    bool GetCurrentSong()
    {
        this->status.clear();
        Client.write(MPD_CURRENTSONG);
        string data = read_data();
        if (data.length() == 0) {
            return false;
        }
        MpdCurrentSong mpd_cs(data);
        auto curfile = mpd_cs.getFile();
        auto l = curfile.length();
        auto p = l <= 26 ? 0 : l - 26;
        auto tail = String(curfile.substr(p).c_str());
        String file = "..." + tail;
        this->status.push_back(file);
        this->status.push_back(" ");
        string name = mpd_cs.getName();
        if (!name.empty()) {
            this->status.push_back(name.c_str());
        }
        this->status.push_back(" ");
        string title = mpd_cs.getTitle();
        if (!title.empty()) {
            this->status.push_back(title.c_str());
        }
        string artist = mpd_cs.getArtist();
        if (!artist.empty()) {
            this->status.push_back(artist.c_str());
        }
        return true;
    }

    bool Stop()
    {
        this->status.clear();
        Client.write(MPD_STOP);
        string data = read_data();
        if (data.length() == 0) {
            return false;
        }
        MpdSimpleCommand mpd_command(data);
        this->status.push_back(mpd_command.GetResult().c_str());
        return true;
    }

    bool Play()
    {
        this->status.clear();
        Client.write(MPD_START);
        string data = read_data();
        if (data.length() == 0) {
            return false;
        }
        MpdSimpleCommand mpd_command(data);
        this->status.push_back(mpd_command.GetResult().c_str());
        return true;
    }

    bool Clear()
    {
        this->status.clear();
        Client.write(MPD_CLEAR);
        string data = read_data();
        if (data.length() == 0) {
            return false;
        }
        MpdSimpleCommand mpd_command(data);
        this->status.push_back(mpd_command.GetResult().c_str());
        return true;
    }

    bool Add_Url(const char* url)
    {
        this->status.clear();
        string add_cmd(MPD_ADD);
        int pos = add_cmd.find("{}");
        add_cmd.replace(pos, 2, url);
        epd_print_topline(add_cmd.c_str());
        Client.write(add_cmd.c_str(), add_cmd.length());
        string data = read_data();
        if (data.length() == 0) {
            return false;
        }
        MpdSimpleCommand mpd_command(data);
        this->status.push_back(mpd_command.GetResult().c_str());
        return true;
    }
};

class MPD_Client {
private:
    MpdConnection con;
    StatusLines status;
    bool playing;
    String show_player(MPD_PLAYER& player);
    void appendStatus(StatusLines& response)
    {
        for (auto line : response) {
            int p = line.indexOf(" - ");
            if (p > -1) {
                String line1 = line.substring(0, p);
                this->status.push_back(line1);
                p += 3;
                if ((p + 1) < line.length()) {
                    String line2 = line.substring(p);
                    this->status.push_back(line2);
                }
            } else {
                this->status.push_back(line);
            }
        }
    }

public:
    MPD_Client()
        : playing(false)
    {
    }
    StatusLines& show_mpd_status();
    StatusLines& toggle_mpd_status();
    StatusLines& play_favourite(const FAVOURITE& fav);
    bool is_playing();
    string GetLastError();
};

extern MPD_Client& mpd;
