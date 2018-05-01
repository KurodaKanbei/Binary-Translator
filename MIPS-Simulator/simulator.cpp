#include<iostream>
#include<fstream>
#include<string>
#include<map>
#include<vector>
#include<deque>
#include<stack>
#include<functional>

const int INF = 0xffffffff;
const int SHF = 0x0000ffff;
const int CHF = 0x000000ff;
const int lo = 32;
const int hi = 33;
const int v0 = 2;
const int a0 = 4;
const int a1 = 5;
typedef long long ll;

using namespace std;

//declaration
class ins;

//global variables for later use
int reg[34];				//place to store register
char data_memory[1 << 25];			//place to store data_memory
int dtp; 					//point to data_memory
int nxt;					//point to next instruction
int label_num;				//total num of labels
fstream fin;
vector<ins> instruction;	//place to store instruction

bool mem_occupied;			//mem is occupied
int reg_occupied[34];		//reg is occupied by n 
bool control_hazard;		//control hazard has appeared.

vector<function<pair<int, int>(ll, ll)>> op_to_func;	//mapping from op to function
map<string, int> rn_to_rp;			//mapping from name of register to place of register
map<string, int> lb_to_la;			//mapping from label to int
map<pair<string, int>, int> in_to_op;			//mapping from name of instruction and num to op;
map<int, int> la_to_r;				//mapping from label to row of ins	

									//function for calculator and controler
pair<int, int> add(ll a, ll b) {
	return pair<int, int>((a + b) & INF, 0);
}

pair<int, int> sub(ll a, ll b) {
	return pair<int, int>((a - b) & INF, 0);
}

pair<int, int> mul(ll a, ll b) {
	ll t = a * b;
	return pair<int, int>(t & INF, (t >> 32) & INF);
}

pair<int, int> divv(ll a, ll b) {
	return pair<int, int>((a / b) & INF, (a % b) & INF);	//(low,high)
}

pair<int, int> xxor(ll a, ll b) {
	return pair<int, int>((a ^ b) & INF, 0);
}

pair<int, int> neg(ll a, ll b) {
	return pair<int, int>((-a) & INF, 0);
}

pair<int, int> seq(ll a, ll b) {
	return pair<int, int>(a == b, 0);
}

pair<int, int> sge(ll a, ll b) {
	return pair<int, int>(a >= b, 0);
}

pair<int, int> sgt(ll a, ll b) {
	return pair<int, int>(a > b, 0);
}

pair<int, int> sle(ll a, ll b) {
	return pair<int, int>(a <= b, 0);
}

pair<int, int> slt(ll a, ll b) {
	return pair<int, int>(a < b, 0);
}

pair<int, int> sne(ll a, ll b) {
	return pair<int, int>(a != b, 0);
}

//class ins used to store instruction briefly
struct ins {
	int op;	
	int rs;	
	int rt;	
	int rd;	
	int sc;	
	int lp;	

	ins() {}		//rs rs -1??
	ins(int op, int rs, int rt, int rd, int sc, int lp) :op(op), rs(rs), rt(rt), rd(rd), sc(sc), lp(lp) {}
	//copy constructor is implicitly called.
};

//class for processing in the pipelining. 
class regulator {
protected:
	ins ii;
	int clock;
public:
	regulator(ins ex) :ii(ex), clock(2) {}
	virtual ~regulator() {};
	int stage() { return clock; }
	virtual int console() = 0;
	virtual bool is_controlor() = 0;
};

class calculator : public regulator {
private:
	function<pair<int, int>(ll, ll)> calc;
	int A;
	int B;
	pair<int, int> ans;

	bool prepare() {
		if (!reg_occupied[ii.rs] && (ii.rt == -1 || !reg_occupied[ii.rt])) {
			calc = op_to_func[ii.op];
			A = reg[ii.rs];
			if (ii.rt == -1) B = ii.sc;
			else B = reg[ii.rt];
			if (ii.rd != -1) reg_occupied[ii.rd] += 1;
			else {
				reg_occupied[lo] += 1;
				reg_occupied[hi] += 1;
			}
			return true;
		}
		else return false;
	}

	void execute() {
		if (ii.op <= 8 || ii.op >= 19)	ans = calc(A, B);
		else ans = calc(unsigned(A), unsigned(B));
	}

	void write_back() {
		if (ii.rd == -1) {
			reg[lo] = ans.first;
			reg[hi] = ans.second;
			reg_occupied[lo] -= 1;
			reg_occupied[hi] -= 1;
		}
		else {
			if (ii.op == 6 || ii.op == 16) reg[ii.rd] = ans.second;
			else reg[ii.rd] = ans.first;
			reg_occupied[ii.rd] -= 1;
		}
	}
public:
	calculator(const ins& ex) :regulator(ex) {}
	int console() {
		int suc = true;
		if (stage() == 2) suc = prepare();
		if (stage() == 3) execute();
		if (stage() == 4) suc = -1;
		if (stage() == 5) write_back();
		if (suc) ++clock;
		return suc;
	}
	bool is_controlor() { return false; }
};

class controlor : public regulator {
private:
	function<pair<int, int>(ll, ll)> calc;
	int A;
	int B;
	bool ans;

	bool prepare() {
		calc = op_to_func[ii.op];
		if (ii.op <= 36) {
			if (!reg_occupied[ii.rs] && (ii.rt <= -1 || !reg_occupied[ii.rt])) {
				A = reg[ii.rs];
				if (ii.rt == -2) B = ii.sc;
				else if (ii.rt == -1) B = 0;
				else B = reg[ii.rt];
				return true;
			}
			return false;
		}
		if (ii.op == 39 || ii.op == 41)
			if (!reg_occupied[ii.rs])
				ii.lp = reg[ii.rs];
			else return false;
			A = B = true;
			return true;
	}		//different from that of calculator

	void execute() {
		ans = calc(A, B).first;
	}

	void write_back() {
		if (ans) {
			if (ii.op == 40 || ii.op == 41) reg[31] = nxt;
			nxt = ii.lp;
		}
	}
public:
	controlor(const ins& ex) :regulator(ex) {}
	int console() {
		int suc = true;
		if (stage() == 2) suc = prepare();
		if (stage() == 3) execute();
		if (stage() == 4) suc = -1;
		if (stage() == 5) write_back();
		if (suc) ++clock;
		return suc;
	}
	bool is_controlor() { return true; }
};

class load : public regulator {
private:
	int pa;		//original address / mem_buffer
	int ans;
	bool prepare() {
		if (ii.rs == -1) {
			reg_occupied[ii.rd] += 1;
			pa = ii.lp;
			return true;
		}
		if (!reg_occupied[ii.rs]) {
			reg_occupied[ii.rd] += 1;
			pa = reg[ii.rs];
			return true;
		}
		return false;
	}

	void execute() {
		if (ii.rs == -1) {
			ans = pa;
		}
		else ans = pa + ii.sc;
	}

	void mem_access() {
		if (ii.op == 42) pa = ans;
		else if (ii.op == 43) pa = (unsigned char)(data_memory[ans]);
		else if (ii.op == 44) {
			pa = int(*((unsigned short*)(data_memory + ans)));
		}
		else if (ii.op == 45) {
			pa = *((int*)(data_memory + ans));
		}
	}
	void write_back() {
		reg[ii.rd] = pa;
		reg_occupied[ii.rd] -= 1;
	}
public:
	load(const ins& ex) :regulator(ex) {}
	int console() {
		int suc = true;
		if (stage() == 2) suc = prepare();
		if (stage() == 3) execute();
		if (stage() == 4) mem_access();
		if (stage() == 5) write_back();
		if (suc) ++clock;
		return suc;
	}
	bool is_controlor() { return false; }
};

class store : public regulator {
private:
	int pa;		//original address / mem_buffer
	int C;		//store data_memory
	int ans;
	bool prepare() {
		if (!reg_occupied[ii.rd]) {
			C = reg[ii.rd];
			if (ii.rs == -1) {
				pa = ii.lp;
				return true;
			}
			if (!reg_occupied[ii.rs]) {
				pa = reg[ii.rs];
				return true;
			}
		}
		return false;
	}

	void execute() {
		if (ii.rs == -1) {
			ans = pa;
		}
		else ans = pa + ii.sc;
	}

	void mem_access() {
		if (ii.op == 46)	data_memory[ans] = C & CHF;
		if (ii.op == 47)	for (int i = 0; i < 2; ++i) data_memory[ans + i] = (C >> (i * 8)) & CHF;
		if (ii.op == 48)	for (int i = 0; i < 4; ++i) data_memory[ans + i] = (C >> (i * 8)) & CHF;
	}
public:
	store(const ins& ex) :regulator(ex) {}
	int console() {
		int suc = true;
		if (stage() == 2) suc = prepare();
		if (stage() == 3) execute();
		if (stage() == 4) mem_access();
		if (suc) ++clock;
		return suc;
	}
	bool is_controlor() { return false; }
};

class mover : public regulator {
private:
	int A;
	bool prepare() {
		if (ii.rs != -1 && reg_occupied[ii.rs])	return false;
		else {
			reg_occupied[ii.rd] += 1;
			if (ii.rs == -1) {
				A = ii.sc;
				return true;
			}
			A = reg[ii.rs];
			return true;
		}
	}

	void write_back() {
		reg[ii.rd] = A;
		reg_occupied[ii.rd] -= 1;
	}
public:
	mover(const ins& ex) :regulator(ex) {}
	int console() {
		int suc = true;
		if (stage() == 2) suc = prepare();
		if (stage() == 4) suc = -1;
		if (stage() == 5) write_back();
		if (suc) ++clock;
		return suc;
	}
	bool is_controlor() { return false; }
};

class nop : public regulator {
public:
	nop(const ins& ex) :regulator(ex) {}
	int console() {
		int suc = true;
		if (stage() == 4) suc = -1;
		++clock;
		return suc;
	}
	bool is_controlor() { return false; }
};

//for string process
int string_to_int(const string& s) {
	int ans = 0;
	if (s[0] == '-') {
		for (int i = 1; i < s.size(); ++i) {
			ans *= 10;
			ans += s[i] - '0';
		}
		return -ans;
	}
	else {
		for (int i = 0; i < s.size(); ++i) {
			ans *= 10;
			ans += s[i] - '0';
		}
		return ans;
	}
}

char to_char(const string& s, int& i) {
	if (s[i] == '\\') {
		++i;
		switch (s[i]) {
		case 'n':	return '\n';
		case 't':	return '\t';
		case '0':	return '\0';
		case '\"':	return '\"';
		default:	throw char();
		}
	}
	else return s[i];
}

class syscall : public regulator {
private:
	int v0_data;		//store the data_memory of $v0
	int a0_data;		//store the data_memory of $a0
	int a1_data;		//store the data_memory of $a1
	string s;

	int i = 0;
	string tmp_s;		//for syscall 8

	bool prepare() {
		if (!reg_occupied[a0] && !reg_occupied[a1] && !reg_occupied[v0]) {
			reg_occupied[v0] += 1;
			v0_data = reg[v0];
			a0_data = reg[a0];
			a1_data = reg[a1];
			return true;
		}
		return false;
	}

	void execute() {
		switch (v0_data) {
		case 1:
			cout << a0_data;
			break;
		case 4:
			while (data_memory[a0_data]) cout << data_memory[a0_data++];
			break;
		case 5:
			getline(cin, s);
			v0_data = string_to_int(s);
			break;
		case 8:
			getline(cin, s);
			i = 0;
			tmp_s = "";
			//	if(s.size() >= a1_data) throw int;
			while (i < s.size())	tmp_s += to_char(s, i), ++i;
			for (int i = 0; i < tmp_s.size(); ++i) 	data_memory[a0_data + i] = tmp_s[i];
			data_memory[a0_data + tmp_s.size()] = '\0';
			v0_data = a0_data;
			break;
		case 9:
			v0_data = dtp;
			dtp += a0_data;
			break;
		case 10:
			exit(0);
		case 17:
			exit(a0_data);
		}			//nedd polilsh
	}

	void write_back() {
		reg[v0] = v0_data;
		reg_occupied[v0] -= 1;
	}
public:
	syscall(const ins& ex) :regulator(ex) {}
	int console() {
		int suc = true;
		if (stage() == 2) suc = prepare();
		if (stage() == 3) execute();
		if (stage() == 4); // need polish
		if (stage() == 5) write_back();
		if (suc) ++clock;
		return suc;
	}
	bool is_controlor() { return true; }
};

//pipelining
class pipeline {
private:
	deque<regulator*> que;
	ins copy;
	int copy_nxt;
	bool copied;		//whether copy is newest

	void push_regulator(const ins& ex) {
		regulator* rl;
		if (ex.op <= 24) rl = new calculator(ex);
		else if (ex.op <= 41) rl = new controlor(ex);
		else if (ex.op <= 45) rl = new load(ex);
		else if (ex.op <= 48) rl = new store(ex);
		else if (ex.op <= 52) rl = new mover(ex);
		else if (ex.op == 53) rl = new nop(ex);
		else rl = new syscall(ex);
		que.push_back(rl);
	}		//process this ins
	void pop_regulator() {
		delete que.front();
		que.pop_front();
	}		//pop finished instruction

	void ins_fecth() {
		if (nxt == instruction.size()) return;
		if (!copied || copy_nxt != nxt) {
			copy = instruction[nxt];  //copy an back-up
			copy_nxt = nxt;
			copied = true;
		}
		if (copied && copy_nxt == nxt) {
			if (!mem_occupied && !control_hazard) {  //delete reg_occupied
				push_regulator(copy);
				++nxt;
				copied = false;
				if ((25 <= copy.op && copy.op <= 41) || copy.op == 54) control_hazard = true;
			}
		}
	}		//instruction fecth stage

	void scan() {
		mem_occupied = false;
		int rec = true;
		for (deque<regulator*>::iterator it = que.begin(); it != que.end(); ++it) {
			rec = (*it)->console();
			if (!rec) break;
			if (rec != -1 && (*it)->stage() == 5) mem_occupied = true;
		}
		if (!que.empty() && (*(que.begin()))->stage() >= 6) {
			pop_regulator();
			if (que.empty() || (*(--que.end()))->is_controlor() == false) control_hazard = false; //*-- ??
		}
		if (rec)	ins_fecth();
	}
public:
	pipeline() :copied(false), copy_nxt(-1) { nxt = la_to_r.at(lb_to_la.at("main")); }
	void console() {
		while (nxt < instruction.size() || !que.empty()) 	scan();
	}
};

//init
void op_to_func_init() {
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(add));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sub));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(mul));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(divv));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(xxor));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(neg));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(divv));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(mul));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(divv));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(add));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(add));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sub));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(mul));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(divv));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(xxor));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(neg));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(divv));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(mul));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(divv));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sge));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sgt));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sle));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(slt));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sne));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sne));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sge));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sle));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sgt));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(slt));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sne));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sgt));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sle));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(sgt));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(slt));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
	op_to_func.push_back(function<pair<int, int>(ll, ll)>(seq));
}

void rn_to_rp_init() {
	rn_to_rp.insert(pair<string, int>("$zero", 0));
	rn_to_rp.insert(pair<string, int>("$at", 1));
	rn_to_rp.insert(pair<string, int>("$v0", 2));
	rn_to_rp.insert(pair<string, int>("$v1", 3));
	rn_to_rp.insert(pair<string, int>("$a0", 4));
	rn_to_rp.insert(pair<string, int>("$a1", 5));
	rn_to_rp.insert(pair<string, int>("$a2", 6));
	rn_to_rp.insert(pair<string, int>("$a3", 7));
	rn_to_rp.insert(pair<string, int>("$t0", 8));
	rn_to_rp.insert(pair<string, int>("$t1", 9));
	rn_to_rp.insert(pair<string, int>("$t2", 10));
	rn_to_rp.insert(pair<string, int>("$t3", 11));
	rn_to_rp.insert(pair<string, int>("$t4", 12));
	rn_to_rp.insert(pair<string, int>("$t5", 13));
	rn_to_rp.insert(pair<string, int>("$t6", 14));
	rn_to_rp.insert(pair<string, int>("$t7", 15));
	rn_to_rp.insert(pair<string, int>("$s0", 16));
	rn_to_rp.insert(pair<string, int>("$s1", 17));
	rn_to_rp.insert(pair<string, int>("$s2", 18));
	rn_to_rp.insert(pair<string, int>("$s3", 19));
	rn_to_rp.insert(pair<string, int>("$s4", 20));
	rn_to_rp.insert(pair<string, int>("$s5", 21));
	rn_to_rp.insert(pair<string, int>("$s6", 22));
	rn_to_rp.insert(pair<string, int>("$s7", 23));
	rn_to_rp.insert(pair<string, int>("$t8", 24));
	rn_to_rp.insert(pair<string, int>("$t9", 25));
	rn_to_rp.insert(pair<string, int>("$k0", 26));
	rn_to_rp.insert(pair<string, int>("$k1", 27));
	rn_to_rp.insert(pair<string, int>("$gp", 28));
	rn_to_rp.insert(pair<string, int>("$sp", 29));
	rn_to_rp.insert(pair<string, int>("$s8", 30));
	rn_to_rp.insert(pair<string, int>("$fp", 30));
	rn_to_rp.insert(pair<string, int>("$ra", 31));

	rn_to_rp.insert(pair<string, int>("$0", 0));
	rn_to_rp.insert(pair<string, int>("$1", 1));
	rn_to_rp.insert(pair<string, int>("$2", 2));
	rn_to_rp.insert(pair<string, int>("$3", 3));
	rn_to_rp.insert(pair<string, int>("$4", 4));
	rn_to_rp.insert(pair<string, int>("$5", 5));
	rn_to_rp.insert(pair<string, int>("$6", 6));
	rn_to_rp.insert(pair<string, int>("$7", 7));
	rn_to_rp.insert(pair<string, int>("$8", 8));
	rn_to_rp.insert(pair<string, int>("$9", 9));
	rn_to_rp.insert(pair<string, int>("$10", 10));
	rn_to_rp.insert(pair<string, int>("$11", 11));
	rn_to_rp.insert(pair<string, int>("$12", 12));
	rn_to_rp.insert(pair<string, int>("$13", 13));
	rn_to_rp.insert(pair<string, int>("$14", 14));
	rn_to_rp.insert(pair<string, int>("$15", 15));
	rn_to_rp.insert(pair<string, int>("$16", 16));
	rn_to_rp.insert(pair<string, int>("$17", 17));
	rn_to_rp.insert(pair<string, int>("$18", 18));
	rn_to_rp.insert(pair<string, int>("$19", 19));
	rn_to_rp.insert(pair<string, int>("$20", 20));
	rn_to_rp.insert(pair<string, int>("$21", 21));
	rn_to_rp.insert(pair<string, int>("$22", 22));
	rn_to_rp.insert(pair<string, int>("$23", 23));
	rn_to_rp.insert(pair<string, int>("$24", 24));
	rn_to_rp.insert(pair<string, int>("$25", 25));
	rn_to_rp.insert(pair<string, int>("$26", 26));
	rn_to_rp.insert(pair<string, int>("$27", 27));
	rn_to_rp.insert(pair<string, int>("$28", 28));
	rn_to_rp.insert(pair<string, int>("$29", 29));
	rn_to_rp.insert(pair<string, int>("$30", 30));
	rn_to_rp.insert(pair<string, int>("$31", 31));
}

void in_to_op_init() {
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("add", 3), 0));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sub", 3), 1));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("mul", 3), 2));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("div", 3), 3));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("xor", 3), 4));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("neg", 2), 5));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("rem", 3), 6));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("mul", 2), 7));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("div", 2), 8));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("addu", 3), 9));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("addiu", 3), 10));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("subu", 3), 11));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("mulu", 3), 12));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("divu", 3), 13));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("xoru", 3), 14));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("negu", 2), 15));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("remu", 3), 16));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("mulu", 2), 17));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("divu", 2), 18));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("seq", 3), 19));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sge", 3), 20));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sgt", 3), 21));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sle", 3), 22));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("slt", 3), 23));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sne", 3), 24));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("beq", 3), 25));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("bne", 3), 26));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("bge", 3), 27));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("ble", 3), 28));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("bgt", 3), 29));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("blt", 3), 30));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("beqz", 2), 31));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("bnez", 2), 32));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("bgez", 2), 33));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("blez", 2), 34));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("bgtz", 2), 35));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("bltz", 2), 36));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("b", 1), 37));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("j", 1), 38));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("jr", 1), 39));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("jal", 1), 40));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("jalr", 1), 41));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("la", 2), 42));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("lb", 2), 43));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("lh", 2), 44));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("lw", 2), 45));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sb", 2), 46));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sh", 2), 47));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sw", 2), 48));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("la", 3), 42));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("lb", 3), 43));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("lh", 3), 44));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("lw", 3), 45));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sb", 3), 46));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sh", 3), 47));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("sw", 3), 48));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("move", 2), 49));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("li", 2), 50));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("mfhi", 1), 51));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("mflo", 1), 52));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("nop", 0), 53));
	in_to_op.insert(pair<pair<string, int>, int>(pair<string, int>("syscall", 0), 54));
}

//offset == 0 can be dismissed. need polish
ins string_to_ins(const string& s) {
	vector<string> ori_ins;
	string tmp;
	for (int i = 0; i < s.size(); ++i) {
		tmp = "";
		while (i < s.size() && s[i] != ' ' && s[i] != ',' && s[i] != '(' && s[i] != ')' && s[i] != '\t') tmp += s[i++];
		if (tmp.size())	ori_ins.push_back(tmp);
	}					//premise : begin with "\t."

	int op;		
	int rs = -1;
	int rt = -1;	
	int rd = -1;			
	int sc = 0;		
	int lp = 0;		
	op = in_to_op.at(pair<string, int>(ori_ins[0], ori_ins.size() - 1));

	if (op <= 24) {
		if (op == 8 || op == 18 || op == 7 || op == 17) {
			rs = rn_to_rp.at(ori_ins[1]);
			if (ori_ins[2][0] == '$') rt = rn_to_rp.at(ori_ins[2]);
			else sc = string_to_int(ori_ins[2]);
		}
		else {
			rd = rn_to_rp.at(ori_ins[1]);
			if (ori_ins[2][0] == '$') rs = rn_to_rp.at(ori_ins[2]);
			else sc = string_to_int(ori_ins[2]);  		//no need to check
			if (ori_ins.size() == 4) {
				if (ori_ins[3][0] == '$') rt = rn_to_rp.at(ori_ins[3]);
				else sc = string_to_int(ori_ins[3]);
			}
		}
	}
	else if (op <= 30) {
		rs = rn_to_rp.at(ori_ins[1]);
		if (ori_ins[2][0] == '$') rt = rn_to_rp.at(ori_ins[2]);
		else sc = string_to_int(ori_ins[2]), rt = -2;
		lp = la_to_r[lb_to_la.at(ori_ins[3])];	//label should already exist
	}
	else if (op <= 36) {
		rs = rn_to_rp.at(ori_ins[1]);
		lp = la_to_r[lb_to_la.at(ori_ins[2])];	//label should already exist
	}
	else if (op == 39 || op == 41) {
		rs = rn_to_rp.at(ori_ins[1]);			//strange
	}
	else if (op <= 41) {
		lp = la_to_r[lb_to_la.at(ori_ins[1])];	//label should already exist
	}
	else if (op <= 48) {
		rd = rn_to_rp.at(ori_ins[1]);
		if (ori_ins.size() == 3) {
			lp = la_to_r[lb_to_la.at(ori_ins[2])];	//label should already exist
		}
		else {
			sc = string_to_int(ori_ins[2]);
			rs = rn_to_rp.at(ori_ins[3]);
		}
	}
	else if (op == 49) {
		rd = rn_to_rp.at(ori_ins[1]);
		rs = rn_to_rp.at(ori_ins[2]);
	}
	else if (op == 50) {
		rd = rn_to_rp.at(ori_ins[1]);
		sc = string_to_int(ori_ins[2]);
	}
	else if (op == 51) {
		rd = rn_to_rp.at(ori_ins[1]);
		rs = hi;
	}
	else if (op == 52) {
		rd = rn_to_rp.at(ori_ins[1]);
		rs = lo;
	}
	return ins(op, rs, rt, rd, sc, lp);
}		//process instruction

//function for data process

void align(int n) {
	int c = 1 << n;
	int d = dtp & -dtp;
	if (d % c == 0)	return;
	while (d < c && dtp > 0) {
		dtp -= d;
		d = dtp & -dtp;
	}
	dtp += c;
}							//only for dtp

void ascii_str(const string& s) {
	for (int i = 0; i < s.size(); ++i) data_memory[dtp++] = to_char(s, i);
}

void asciiz_str(const string& s) {
	ascii_str(s);
	data_memory[dtp++] = '\0';
}

void byte(int num) {
	data_memory[dtp++] = num & CHF;
}

void half(int num) {
	data_memory[dtp++] = num & CHF;
	data_memory[dtp++] = (num >> 8) & CHF;
}

void word(int num) {
	for (int i = 0; i < 4; ++i)	data_memory[dtp++] = (num >> (i * 8)) & CHF;
}

void space(int n) {
	dtp += n;
}

void data_process(const string& s) {
	vector<string> data_ins;
	string tmp;
	int i = 0;
	for (i = 0; i < s.size(); ++i) {
		tmp = "";
		while (i < s.size() && s[i] != ' ' && s[i] != ',' && s[i] != '\t' && s[i] != '.') tmp += s[i++];  	//premise : begin with "\t."
		if (tmp.size()) {
			data_ins.push_back(tmp);
			break;
		}
	}
	if (data_ins[0][1] == 's') {
		tmp = "";
		while (s[i] != '\"') ++i; ++i;
		while (i < s.size() - 1) tmp += s[i++];
		data_ins.push_back(tmp);
	}
	else {
		for (; i < s.size(); ++i) {
			tmp = "";
			while (i < s.size() && s[i] != ' ' && s[i] != ',' && s[i] != '\t')	tmp += s[i++];
			if (tmp.size()) data_ins.push_back(tmp);
		}
	}
	if (data_ins[0][1] == 'l')			align(string_to_int(data_ins[1]));	//.align
	else if (data_ins[0][1] == 'y')
		for (int i = 1; i < data_ins.size(); ++i)
			byte(string_to_int(data_ins[i]));								//.byte
	else if (data_ins[0][1] == 'a')
		for (int i = 1; i < data_ins.size(); ++i)
			half(string_to_int(data_ins[i]));								//.half
	else if (data_ins[0][1] == 'o')
		for (int i = 1; i < data_ins.size(); ++i)
			word(string_to_int(data_ins[i]));								//.word
	else if (data_ins[0][1] == 'p')		space(string_to_int(data_ins[1]));	//.space
	else if (data_ins[0].size() == 5)	ascii_str(data_ins[1]);				//ascii
	else								asciiz_str(data_ins[1]);			//asciiz
}

bool is_data(const string& s) {
	return s[0] == '.' && s[1] == 'd';
}

bool is_text(const string& s) {
	return s[0] == '.' && s[1] == 't';
}

bool is_label(const string& s) {
	return s[s.size() - 1] == ':';
}

class read_in {
private:
	fstream in;
	stack<string> label_stack;
	vector<string> s;
	string tmp_s;
	bool status;		// 0 .data_memory  / 1 .text
public:
	read_in(const string& ex) :in(ex) {}
	void process() {
		while (getline(in, tmp_s)) {
			int i = 0;
			while (tmp_s[i] == '\t' || tmp_s[i] == ' ')	++i;
			tmp_s = tmp_s.substr(i, tmp_s.size() - i);
			if (!tmp_s.size())	continue;

			if (is_data(tmp_s)) {
				status = false;
				continue;
			}
			if (is_text(tmp_s)) {
				status = true;
				continue;
			}
			if (!status)
				if (is_label(tmp_s)) {
					la_to_r.insert(pair<int, int>(lb_to_la.size(), dtp));
					lb_to_la.insert(pair<string, int>(tmp_s.substr(0, tmp_s.size() - 1), lb_to_la.size()));
				}
				else	data_process(tmp_s);
			else if (is_label(tmp_s)) {
				label_stack.push(tmp_s.substr(0, tmp_s.size() - 1));
			}
			else {
				if (tmp_s[0] == '#') continue;
				while (!label_stack.empty()) {
					la_to_r.insert(pair<int, int>(lb_to_la.size(), s.size()));
					lb_to_la.insert(pair<string, int>(label_stack.top(), lb_to_la.size()));
					label_stack.pop();
				}
				s.push_back(tmp_s);
			}
		}
		for (int i = 0; i < s.size(); ++i) instruction.push_back(string_to_ins(s[i]));
	}
};

int main(int argc, char *argv[]) {
	reg[29] = 1 << 25 - 1;
	op_to_func_init();
	rn_to_rp_init();
	in_to_op_init();

	read_in read(argv[1]);
	read.process();

	pipeline simulator;
	simulator.console();
	return 0;
}
