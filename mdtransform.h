#ifndef _MDTRANSFORM_H_
#define _MDTRANSFORM_H_

#include <string>
using namespace std;

typedef struct Cnode {
    vector <Cnode *> ch;
    string heading;
    string tag;
    Cnode (const string &hd): heading(hd) {}
} Cnode;

typedef struct node {
    int type;
    vector <node *> ch;
    string elem[3]; /* elem[1] --> contents;  elem[2] --> link;   elem[3] --> title */

    node (int _type): type(_type) {}
} node;

enum {
    maxLength        = 10000,
    nul              = 0,
    paragraph        = 1,
    href             = 2,
    ul               = 3,
    ol               = 4,
    li               = 5,
    em               = 6,
    strong           = 7,
    hr               = 8,
    br               = 9,
    image            = 10,
    quote            = 11,
    h1               = 12,
    h2               = 13,
    h3               = 14,
    h4               = 15,
    h5               = 16,
    h6               = 17,
    blockcode        = 18,
    code             = 19,
};

const std::string frontTag[] = {
    "", "<p>", "", "<ul>", "<ol>", "<li>", "<em>", "<strong>",
    "<hr color=#CCCCCC size=1 />", "<br />",
    "", "<blockquote>",
    "<h1 ", "<h2 ", "<h3 ", "<h4 ", "<h5 ", "<h6 ", // attention the space in the end
    "<pre><code>", "<code>",
};

const std::string backTag[] = {
    "","</p>","","</ul>","</ol>","</li>","</em>","</strong>",
    "","","","</blockquote>",
    "</h1>","</h2>","</h3>","</h4>","</h5>","</h6>",
    "</code></pre>","</code>",
};


class MarkdownTransform
{
public:
    MarkdownTransform(const std::string &filename);
    std::string getTableOfContents() { return TOC; }
    std::string getContents() { return content; }

    ~MarkdownTransform() {
        destroy<node>(root);
        destroy<Cnode>(Croot);
    }

private:
    string content, TOC;
    node *root, *now;
    Cnode *Croot;
    int cntTag = 0;
    char s[maxLength];


    inline pair<int, char *> start(char *src)
    {
        if ((int)strlen(src) == 0)
            return make_pair(0, nullptr);
        
        unsigned int cntspace = 0, cnttab = 0;
        for (unsigned int i = 0; src[i] != '\0'; i++) {
            if (src[i] == ' ') {
                cntspace++;
            } else if (src[i] == '\t') {
                cnttab++;
            }
            return make_pair(cnttab + cntspace / 4, src + i);
        }

        return make_pair(0, nullptr);
    }

    inline pair<int, char *> JudgeType(char *src)
    {
        char *ptr = src;

        while (*ptr == '#')
            ptr++;
        
        if (ptr - src > 0 && *ptr == ' ')
            return make_pair(ptr - src + h1 - 1, ptr + 1);

        ptr = src;

        if (strncmp(ptr, "```", 3) == 0) {
            return make_pair(blockcode, ptr + 3);
        }

        if (/* (strncmp(ptr, "+ ", 2) == 0) || */ (strncmp(ptr, "- ", 2) == 0) || (strncmp(ptr, "* ", 2) == 0) ) {
            return make_pair(ul, ptr + 1);
        }

        if (*ptr == '>' && (ptr[1] == ' ')) {
            return make_pair(quote, ptr + 1);
        }

        char *ptr1 = ptr;
        while (*ptr1 && (isdigit(*ptr1))) ptr1++;
        if (ptr1 != ptr && *ptr1 == '.' && ptr1[1] == ' ') {
            return make_pair(ol, ptr1 + 1);
        }

        return make_pair(paragraph, ptr);
    }

    inline bool isHeading(node *v) {
        return (v->type >= h1 && v->type <= h6);
    }

    inline bool isImage(node *v) {
        return (v->type == image);
    }

    inline bool isHref(node *v) {
        return (v->type == href);
    }

    inline node *findnode(int depth) {
        node *ptr = root;
        while (!ptr->ch.empty() && depth != 0) {
            ptr = ptr->ch.back();
            if (ptr->type == li) {
                depth--;
            }
        }

        return ptr;
    }


    void Cins(Cnode *v, int x, const string &hd, int tag) {
        int n = (int)v->ch.size();
        if (x == 1) {
            v->ch.push_back(new Cnode(hd));
            v->ch.back()->tag = "tag" + to_string(tag);
            return ;
        }

        if (!n || v->ch.back()->heading.empty()) {
            v->ch.push_back(new Cnode(""));
        }

        Cins(v->ch.back(), x - 1, hd, tag);
    }

    void insert(node *v, const string &src) {
        int n = (int)src.size();
        bool incode = false,
            inem = false,
            instrong = false,
            inautolink = false;
        v->ch.push_back(new node(nul));

        for (unsigned int i = 0; i < n; i++) {
            char ch = src[i];
            if (ch == '\\') {
                ch = src[++i];
                v->ch.back()->elem[0] += string(1, ch);
                continue;
            }

            if (ch == '`' && !inautolink) {
                incode ? v->ch.push_back(new node(nul)) : v->ch.push_back(new node(code));
                incode = !incode;
                continue;
            }

            /* deal with em */
            if (ch == '*' && (i < n-1 && (src[i + 1] == '*')) && !incode && !inautolink) {
                i++;
                instrong ? v->ch.push_back(new node(nul)) : v->ch.push_back(new node(strong));
                instrong = !instrong;
                continue;
            }

            if (ch == '_' && !incode && !instrong && !inautolink) {
                inem ? v->ch.push_back(new node(nul)) : v->ch.push_back(new node(em));
                inem = !inem;
                continue;
            }

            /* deal with picture */
            if (ch == '!' && (i < n - 1 && src[i + 1] == '[')
                && !incode && !instrong && !inem && !inautolink)
            {
                v->ch.push_back(new node(image));
                for (i += 2; i < n - 1 && src[i] != ']'; i++) {
                    v->ch.back()->elem[0] += string(1, src[i]);
                }
                i++;
                for (i++; i < n - 1 && src[i] != ' ' && src[i] != ')'; i++) {
                    v->ch.back()->elem[1] += string(1, src[i]);
                }
                if (src[i] != ')') {
                    for (i++; i < n - 1 && src[i] != ')'; i++) {
                        if (src[i] != '"')
                            v->ch.back()->elem[2] += string(1, src[i]);
                    }
                }
                v->ch.push_back(new node(nul));
                continue;
            }

            /* deal with hyperlink */
            if (ch == '[' && !incode && !instrong && !inem && !inautolink) {
                v->ch.push_back(new node(href));
                for (i++; i < n - 1 && src[i] != ']'; i++) {
                    v->ch.back()->elem[0] += string(1, src[i]);
                }
                i++;
                for (i++; i < n - 1 && src[i] != ' ' && src[i] != ')'; i++) {
                    v->ch.back()->elem[1] += string(1, src[i]);
                }
                if (src[i] != ')') {
                    for (i++; i < n - 1 && src[i] != ')'; i++) {
                        if (src[i] != '"')
                            v->ch.back()->elem[2] += string(1, src[i]);
                    }
                }
                v->ch.push_back(new node(nul));
                continue;
            }

            v->ch.back()->elem[0] += string(1, ch);
            if (inautolink)
                v->ch.back()->elem[1] += string(1, ch);
        }
        if (src.size() >= 2) {
            if (src.at(src.size() - 1) == ' ' && src.at(src.size() - 2) == ' ')
                v->ch.push_back(new node(br));
        }
    }

    inline bool isCutline(char *src) {
        int cnt = 0;
        char *ptr = src;
        while (*ptr) {
            if (*ptr != ' ' && *ptr != '\t' && *ptr != '-')
                return false;
            if (*ptr == '-')
                cnt++;
            ptr++;
        }

        return (cnt >= 3);
    }

    inline void mkpara(node *v) {
        if (v->ch.size() == 1u && v->ch.back()->type == paragraph)
            return ;
        if (v->type == paragraph)
            return ;
        if (v->type == nul) {
            v->type = paragraph;
            return ;
        }
        node *x = new node(paragraph);
        x->ch = v->ch;
        v->ch.clear();
        v->ch.push_back(x);
    }


    void dfs(node *v) {
        if (v->type == paragraph && v->elem[0].empty() && v->ch.empty())
            return ;

        content += frontTag[v->type];
        bool flag = true;

        if (isHeading(v)) {
            content += "id=\"" + v->elem[0] + "\">";
            flag = false;
        }
        if (isHref(v)) {
            content += "<a href=\"" + v->elem[1] + "\" title=\"" + v->elem[2] + "\">" + v->elem[0] + "</a>";
            flag = false;
        }
        if (isImage(v)) {
            content += "<img alt=\"" + v->elem[0] + "\" src=\"" + v->elem[1] + "\" title=\"" + v->elem[2] + "\" />";
            flag = false;
        }

        if (flag) {
            content += v->elem[0];
            flag = false;
        }

        for (unsigned int i = 0; i < (int)v->ch.size(); i++) {
            dfs(v->ch[i]);
        }

        content += backTag[v->type];
    }

    void Cdfs(Cnode *v, string index) {
        TOC += "<li>\n";
        TOC += "<a href=\"#" + v->tag + "\">" + index + " " + v->heading + "</a>\n";
        int n = (int)v->ch.size();
        if (n) {
            TOC += "<ul>\n";
            for (int i = 0; i < n; i++) {
                Cdfs(v->ch[i], index + to_string(i + 1) + ".");
            }
            TOC += "</ul>\n";
        }
        TOC += "</li>\n";
    }

    template <typename T>
    void destroy(T *v) {
        for (int i = 0; i < (int)v->ch.size(); i++) {
            destroy(v->ch[i]);
        }
        delete v;
    }
    
}; /* class end */




MarkdownTransform::MarkdownTransform(const std::string &filename) {
    Croot = new Cnode("");
    root = new node(nul);
    now = root;

    std::ifstream fin(filename);

    bool newpara = false;
    bool inblock = false;

    while (!fin.eof()) {
        fin.getline(s, maxLength);

        if (!inblock && isCutline(s)) {
            now = root;
            now->ch.push_back(new node(hr));
            newpara = false;
            continue;
        }

        std::pair<int, char *> ps = start(s);

        if (!inblock && ps.second == nullptr) {
            now = root;
            newpara = true;
            continue;
        }

        std::pair<int, char *> tj = JudgeType(ps.second);

        if (tj.first == blockcode) {
            inblock ? now->ch.push_back(new node(nul)) : now->ch.push_back(new node(blockcode));
            inblock = !inblock;
            continue;
        }

        if (inblock) {
            now->ch.back()->elem[0] += string(s) + '\n';
            continue;
        }

        if (tj.first == paragraph) {
            if (now == root) {
                now = findnode(ps.first);
                now->ch.push_back(new node(paragraph));
                now = now->ch.back();
            }
            bool flag = false;
            if (newpara && !now->ch.empty()) {
                node * ptr = nullptr;
                for (auto i : now->ch) {
                    if (i->type == nul)
                        ptr = i;
                }
                if (ptr != nullptr)
                    mkpara(ptr);
                flag = true;
            }
            if (flag) {
                now->ch.push_back(new node(paragraph));
                now = now->ch.back();
            }
            now->ch.push_back(new node(nul));
            insert(now->ch.back(), string(tj.second));
            newpara = false;
            continue;
        }

        now = findnode(ps.first);

        if (tj.first >= h1 && tj.first <= h6) {
            now->ch.push_back(new node(tj.first));
            now->ch.back()->elem[0] = "tag" + to_string(++cntTag);
            insert(now->ch.back(), string(tj.second));
            Cins(Croot, tj.first - h1 + 1, string(tj.second), cntTag);
        }

        if (tj.first == ul) {
            if (now->ch.empty() || now->ch.back()->type != ul) {
                now->ch.push_back(new node(ul));
            }
            now = now->ch.back();
            bool flag = false;
            if (newpara && !now->ch.empty()) {
                node *ptr = nullptr;
                for (auto i : now->ch) {
                    if (i->type == li)
                        ptr = i;
                }
                if (ptr != nullptr)
                    mkpara(ptr);
                flag = true;
            }
            now->ch.push_back(new node(li));
            now = now->ch.back();
            if (flag) {
                now->ch.push_back(new node(paragraph));
                now = now->ch.back();
            }
            insert(now, string(tj.second));
        }

        if (tj.first == ol) {
            if (now->ch.empty() || now->ch.back()->type != ol) {
                now->ch.push_back(new node(ol));
            }
            now = now->ch.back();
            bool flag = false;
            if (newpara && !now->ch.empty()) {
                node *ptr = nullptr;
                for (auto i : now->ch) {
                    if (i->type == li)
                        ptr = i;
                }
                if (ptr != nullptr)
                    mkpara(ptr);
                flag = true;
            }
            now->ch.push_back(new node(li));
            now = now->ch.back();
            if (flag) {
                now->ch.push_back(new node(paragraph));
                now = now->ch.back();
            }
            insert(now, string(tj.second));
        }

        if (tj.first == quote) {
            if (now->ch.empty() || now->ch.back()->type != quote) {
                now->ch.push_back(new node(quote));
            }
            now = now->ch.back();
            if (newpara || now->ch.empty()) {
                now->ch.push_back(new node(paragraph));
            }
            insert(now->ch.back(), string(tj.second));
        }

        newpara = false;
    }

    fin.close();
    dfs(root);

    TOC += "<ul>";
    for (int i = 0; i < (int)Croot->ch.size(); i++)
        Cdfs(Croot->ch[i], to_string(i + 1) + ".");
    TOC += "</ul>";
}


#endif /* _MDTRANSFORM_H */
