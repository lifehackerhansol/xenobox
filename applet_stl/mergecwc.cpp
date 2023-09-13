// vector is the easiest way to store this kind of data.

#include <vector>
#include <string>

#include "../xenobox.h"

// #define STANDALONE

#ifdef STANDALONE
// #include <iostream> //debug
char* myfgets(char* buf, int n, FILE* fp)
{ // accepts LF/CRLF
    char* ret = fgets(buf, n, fp);
    if (!ret)
        return NULL;
    if (strlen(buf) && buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = 0;
    if (strlen(buf) && buf[strlen(buf) - 1] == '\r')
        buf[strlen(buf) - 1] = 0;
    return ret;
}
unsigned char buf[BUFLEN];
#endif

typedef std::pair<std::pair<int, std::string>, // enabled? / cheat title
                  std::vector<u32>             // actual cheat
                  >
    cwcheat;

typedef std::pair<std::pair<std::string, std::string>, // game ID / game name
                  std::vector<cwcheat>>
    cwgamecheat;

typedef std::vector<cwgamecheat> cwcheatdb;

static int line;
// 0: OK -1:End -2:Ignore 1:Error
static int cwreadline(FILE* f, std::string& cmd, std::string& arg1, std::string& arg2)
{
    char* p;
    cmd = "", arg1 = "", arg2 = "";
    *cbuf = 0;
    while (!*cbuf)
    {
        line++;
        if (!myfgets(cbuf, 1024, f))
            return -1;
    }
    cmd = strtok(cbuf, " \t");
    if (cmd == "_S" || cmd == "_G" || cmd == "_C0" || cmd == "_C1")
    {
        // cout<<cmd<<endl;
        p = cbuf + strlen(cbuf) + 1;
        for (; *p == ' ' || *p == '\t'; p++)
            if (!*p)
                return 1;
        arg1 = p;
        // cout<<arg1<<endl;
        return 0;
    }
    else if (cmd == "_L")
    {
        // cout<<cmd<<endl;
        p = strtok(NULL, " \t");
        if (!p)
            return -2;
        arg1 = p;
        p = strtok(NULL, " \t");
        if (!p)
            return -2;
        arg2 = p;
        // cout<<arg2<<endl;
        return 0;
    }
    else
        return 1;
}

static int cwparse(FILE* f, cwcheatdb& db)
{ // parse usrcheat
    std::string cmd, arg1, arg2;
    std::string gameID = "", gamename = "", cheatname = "";
    int cheat_enable = 0;
    cwgamecheat g;
    std::vector<u32> v;
    int first_db = 1, first_game = 1, r;

    line = 0;
    // execute state machine!
cwparse_loop:
    if ((r = cwreadline(f, cmd, arg1, arg2)) > 0)
        return -1;
    // cout<<cmd<<endl;
    if (r == -1)
    {
        // flush v
        // if(v.empty())return 1; //allows (broken) null entry bah.
        g.second.push_back(make_pair(make_pair(cheat_enable, cheatname), v));

        g.first.first = gameID, g.first.second = gamename;
        db.push_back(g);
        g.second.clear();
        v.clear();
        return 0; // EOF!
    }
    else if (r == -2)
    {
        // should I show warning thingy?
        goto cwparse_loop; // line malformed (sorry, cannot store)
    }
    else if (cmd == "_S")
    {
        if (!first_db)
        {
            // flush v
            // if(v.empty())return 1; //allows (broken) null entry bah.
            g.second.push_back(make_pair(make_pair(cheat_enable, cheatname), v));

            g.first.first = gameID, g.first.second = gamename;
            db.push_back(g);
            g.second.clear();
            v.clear();
            cheatname = "";
            first_game = 1;
        }
        first_db = 0;
        gameID = arg1; // won't fix hyphen... I should care homebrew
        if (cwreadline(f, cmd, arg1, arg2))
            return -1;
        if (cmd != "_G")
            return 1;
        gamename = arg1;
    }
    else if (cmd == "_G")
    {
        if (!first_db)
        {
            // flush v
            // if(v.empty())return 1;
            g.second.push_back(make_pair(make_pair(cheat_enable, cheatname), v));

            g.first.first = gameID, g.first.second = gamename;
            db.push_back(g);
            g.second.clear();
            v.clear();
            cheatname = "";
            first_game = 1;
        }
        first_db = 0;
        gamename = arg1; // won't fix hyphen... I should care homebrew
        if (cwreadline(f, cmd, arg1, arg2))
            return -1;
        if (cmd != "_S")
            return 1;
        gameID = arg1;
    }
    else if (cmd == "_C0" || cmd == "_C1")
    {
        if (gamename == "" || gameID == "")
            return 1;
        if (!first_game)
        {
            // flush v
            // if(v.empty())return 1;
            g.second.push_back(make_pair(make_pair(cheat_enable, cheatname), v));
            v.clear();
        }
        first_game = 0;
        cheat_enable = 0;
        if (cmd[2] == '1')
            cheat_enable = 1;
        cheatname = arg1;
    }
    else if (cmd == "_L")
    {
        if (cheatname == "")
            return 1;
        v.push_back(strtol(arg1.c_str(), NULL, 0));
        v.push_back(strtol(arg2.c_str(), NULL, 0));
    }
    else
        return 1;
    goto cwparse_loop;
}

static int cwoutput(FILE* f, cwcheatdb& db)
{ // output cheat.db
    unsigned int i, j, k;
    for (i = 0; i < db.size(); i++)
    {
        cwgamecheat sg = db[i]; // must not use &, because src will be destructed later.
        fprintf(f, "_S %s\n_G %s\n", sg.first.first.c_str(), sg.first.second.c_str());
        for (j = 0; j < sg.second.size(); j++)
        {
            cwcheat& sC = sg.second[j];
            fprintf(f, "_C%d %s\n", sC.first.first, sC.first.second.c_str());
            for (k = 0; k < sC.second.size(); k += 2)
            {
                fprintf(f, "_L 0x%08X 0x%08X\n", sC.second[k], sC.second[k + 1]);
            }
        }
        // fputs("\n",f);
    }
    return 0;
}

static int cwmerge(cwcheatdb& dst, cwcheatdb& src)
{ // merge DB
    unsigned int i, j, l, m;
    for (i = 0; i < src.size(); i++)
    {
        cwgamecheat sg = src[i]; // must not use &, because src will be destructed later.
        for (j = 0; j < dst.size(); j++)
        {
            if (dst[j].first.first == sg.first.first)
                break;
        }
        if (j == dst.size())
        {
            dst.push_back(sg);
            continue;
        } // merge game

        // merge content
        cwgamecheat& dg = dst[j];
        for (l = 0; l < sg.second.size(); l++)
        {
            cwcheat& sC = sg.second[l];
            for (m = 0; m < dg.second.size(); m++)
            {
                cwcheat& dC = dg.second[m];
                if (sC.second == dC.second)
                    break; // found cheat
            }
            if (m == dg.second.size())
                dg.second.push_back(sC);
        }
    }
    return 0;
}

#ifndef STANDALONE
extern "C" int mergecwc(const int argc, const char** argv)
{
#else
extern "C" int main(const int argc, const char** argv)
{
#endif
    FILE *f = NULL, *out = NULL;
    if (argc < 3)
    {
        fprintf(stderr, "mergecwc target.db cheat.db merge.db...\n");
        return 1;
    }

    const int I = 1; // target
    int i = 2;
    if (!(f = fopen(argv[i], "rb")))
    {
        fprintf(stderr, "cannot open base cheat file\n");
        return 2;
    }
    i++;

    cwcheatdb usr, _merge;
    int ret = 0;
    fprintf(stderr, "Reading %s... ", argv[i + 1]);
    if (ret = cwparse(f, usr))
    {
        fprintf(stderr, "Failed.\n");
        return ret;
    }
    fprintf(stderr, "Done.\n");
    fclose(f);

    for (; i < argc; i++)
    {
        fprintf(stderr, "Reading %s... ", argv[i]);
        if (!(f = fopen(argv[i], "rb")))
        {
            fprintf(stderr, "cannot open.\n");
            continue;
        }
        if (cwparse(f, _merge))
        {
            fprintf(stderr, "Failed.\n");
            fclose(f);
            continue;
        }
        fprintf(stderr, "Done.\n");
        fclose(f);
        cwmerge(usr, _merge);
    }
    if (!(out = fopen(argv[I], "wb")))
    {
        fprintf(stderr, "cannot open target cheat file\n");
        return 2;
    }
    fprintf(stderr, "Writing new cheatdb... ");
    cwoutput(out, usr);
    fprintf(stderr, "Done.\n");
    fclose(out);
    fprintf(stderr, "Everything is Ok.\n");
    return ret;
}
