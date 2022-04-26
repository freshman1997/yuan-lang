#include <unordered_set>

#include "lex.h"

const Token & TokenReader::peek()
{
    return tokens[idx];
}

void TokenReader::set_pos(int i)
{
    if (i < 0 || i >= tokens.size() || tokens.empty()) return;
    this->idx = i;
}

const Token & TokenReader::get_and_read()
{
    return tokens[idx++];
}

void TokenReader::unread()
{
    --idx;
}

void TokenReader::consume()
{
	++idx;
}

const char * TokenReader::get_file_name() const
{
    return filename;
}

void TokenReader::set_file_handle(FILE *fd)
{
    this->file = fd;
}

void TokenReader::add_token(Token &token)
{
    tokens.push_back(token);
}

int TokenReader::_sz() const
{
    return sz;
}

void TokenReader::set_file_name(const char *fn)
{
    this->filename = fn;
}

void TokenReader::set_sz(int sz)
{
    this->sz = sz;
}

Token & TokenReader::get_last()
{
    return tokens.back();
}

Token * TokenReader::peek_last_one()
{
	return idx - 1 >= 0 ? &tokens[idx - 1] : NULL;
}

void TokenReader::set_content(char *content)
{
    this->content = content;
}

char * TokenReader::get_content()
{
    return this->content;
}

int TokenReader::get_pos()
{	
	return idx;
}

const Token & TokenReader::get(int i)
{
	if (i >= tokens.size()) {
		error("index over", filename);
	}
	return tokens[i];
}

static char * read_all_the_file(const char *filename, TokenReader &reader)
{
    FILE *fd = NULL;
    if (!(fd = fopen(filename, "rb"))) {
        error("can not open file: ", filename);
    }

    int res = fseek(fd, 0, SEEK_END);
    if (res) {
        error("seek file fail: ", filename);
    }

    int length = ftell(fd);
    res = fseek(fd, 0, SEEK_SET);

	char* buf = new char[length + 1];
	std::memset(buf, 0, length);
    res = fread(buf, sizeof(char), length, fd);
    if (res != length) {
        error("read file fail: ", filename);
    }

    buf[length] = '\0';
    reader.set_file_handle(fd);
    reader.set_sz(length);
	reader.set_file_name(filename);

	return buf;
}

static bool is_keyword(const Token& tok) {
	static const char* kw[] = {
	  "return", "if", "else", "for", "while", "do", "fn", "local",
	  "switch", "case", "default", "break", "continue", "require", "to", "goto", "in", "nil"
	};

	static unordered_set<string> s;
	// 先直接遍历
	if (s.empty()) {
		for (int i = 0; i < sizeof(kw) / sizeof(*kw); ++i)
			s.insert(kw[i]);
	}
	char* p = tok.from;
	string t;
	int i = 0;
	for (; i < tok.len; ++i) t.push_back(p[i]);
	return s.count(t);
}

static int read_ident(char* s)
{
	int len = 0;
	while (!isspace((unsigned char)s[len]) && !ispunct((unsigned char)s[len])) {
		len++;
	}
	return len;
}

// Read a punctuator token from p and returns its length.
static int read_punct(char* p) 
{
	static const char* kw[] = {
	  "<<=", ">>=", "...", "==", "!=", "<=", ">=", "+=",
	  "-=", "*=", "/=", "++", "--", "%=", "&=", "|=", "^=", "&&",
	  "||", "<<", ">>", ":", "``", "(", ")", "{", "}", "[", "]", ";",
	};

	for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
		if (startswith(p, kw[i]))
			return int(strlen(kw[i]));

	return ispunct(*p) ? 1 : 0;
}

// Find a closing double-quote.
static char* string_literal_end(char* p) 
{
	for (; *p != '"'; p++) {
		if (*p == '\0') {
			cout << "unclosed string literal" << endl;
			exit(1);
		}
	}
	return p;
}

static int from_hex(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('a' <= c && c <= 'f')
		return c - 'a' + 10;
	return c - 'A' + 10;
}

static int read_escaped_char(char** new_pos, char* p) {
	if ('0' <= *p && *p <= '7') {
		// Read an octal number.
		int c = *p++ - '0';
		if ('0' <= *p && *p <= '7') {
			c = (c << 3) + (*p++ - '0');
			if ('0' <= *p && *p <= '7')
				c = (c << 3) + (*p++ - '0');
		}
		*new_pos = p;
		return c;
	}

	if (*p == 'x') {
		// Read a hexadecimal number.
		p++;
		if (!isxdigit(*p)) {
			cout << "invalid hex escape sequence" << endl;
			exit(1);
		}

		int c = 0;
		for (; isxdigit(*p); p++)
			c = (c << 4) + from_hex(*p);
		*new_pos = p;
		return c;
	}

	*new_pos = p + 1;

	// Escape sequences are defined using themselves here. E.g.
	// '\n' is implemented using '\n'. This tautological definition
	// works because the compiler that compiles our compiler knows
	// what '\n' actually is. In other words, we "inherit" the ASCII
	// code of '\n' from the compiler that compiles our compiler,
	// so we don't have to teach the actual code here.
	//
	// This fact has huge implications not only for the correctness
	// of the compiler but also for the security of the generated code.
	// For more info, read "Reflections on Trusting Trust" by Ken Thompson.
	// https://github.com/rui314/chibicc/wiki/thompson1984.pdf
	switch (*p) {
	case 'a': return '\a';
	case 'b': return '\b';
	case 't': return '\t';
	case 'n': return '\n';
	case 'v': return '\v';
	case 'f': return '\f';
	case 'r': return '\r';
		// [GNU] \e for the ASCII escape character is a GNU C extension.
	case 'e': return 27;
	default: return *p;
	}
}

static bool check_number_string(char *from, int len)
{
	
	return false;
}

static void add_token(TokenReader &reader, char *from, int len, TokenType type, int row, int col, char *str)
{
    Token t = {type, from, len, row, col, str, 0};
	if (type == TokenType::num) {
		char *num = new char[len + 1];
		memcpy(num, from, len);
		num[len] = '\0';
		t.val = atof(num);
	}
    reader.add_token(t);
}

static int read_string_literal(char* start, char* quote, int line, const char *filename, TokenReader &reader) {
	char* end = string_literal_end(quote + 1);
	char* buf = new char[1 + (end - quote)]; //calloc(1, end - quote);
	int len = 0;
	if (int(end - quote) > 0) {
		for (char* p = quote + 1; p < end;) {
			if (*p == '\\')
				buf[len++] = read_escaped_char(&p, p + 1);
			else
				buf[len++] = *p++;
		}
	}

	buf[len] = '\0';

	add_token(reader, start + 1, len, TokenType::str, line, 0, buf);

	return int(end - quote) + 1; // skip last "
}

void tokenize(const char *filename, TokenReader &reader)
{
    char *content = read_all_the_file(filename, reader);
    if (!content) {
        error("can not read file: ", filename);
    }

    char *p = content;
    if (!memcmp(p, "\xef\xbb\xbf", 3))
		p += 3;

    int line = 0, col = 0;
    while(*p != '\0') {
        // Skip line comments.
		if (startswith(p, "//")) {
			p += 2;
			while (*p != '\n')
				p++;
			continue;
		}

		// Skip block comments.
		if (startswith(p, "/*")) {
			int st = 0;
			while (*p)
			{
				if (*p == '*') st++;
				else if (st && *p != '/') st = 0;
				else if (st && *p == '/') break;
				else if (*p == '\n') line++;
				p++;
			}
			++p;
			continue;
		}
        
        if (*p == '\r') {
            ++p;
            continue;
        }

		// Skip newline.
		if (*p == '\n') {
			p++;
			++line;
			continue;
		}

		// Skip whitespace characters.
		if (isspace((unsigned char)*p)) {
			p++;
			continue;
		}

		// Numeric literal
		if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1]))) {
			char* q = p++;
			for (;;) {
				if (p[0] && p[1] && strchr("eEpP", p[0]) && strchr("+-", p[1]))
					p += 2;
				else if (isalnum(*p) || *p == '.')
					p++;
				else
					break;
			}
	        add_token(reader, q, int(p - q), TokenType::num, line, 0, NULL);
			continue;
		}

		// String literal
		if (*p == '"') {
			p += read_string_literal(p, p, line, filename, reader);
			//cout << *p << endl;
			continue;
		}

		// Identifier or keyword
		int ident_len = read_ident(p);
		if (ident_len) {
	        add_token(reader, p, ident_len, TokenType::iden, line, 0, NULL);
			if (is_keyword(reader.get_last())) reader.get_last().type = TokenType::keyword;
			p += ident_len;
			continue;
		}

		// Punctuators
		int punct_len = read_punct(p);
		if (punct_len) {
	        add_token(reader, p, punct_len, TokenType::sym, line, 0, NULL);
			p += reader.get_last().len;
			continue;
		}
    }
    add_token(reader, p, 0, TokenType::eof, -1, -1, NULL);
}

