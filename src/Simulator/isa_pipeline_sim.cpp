// isa_pipeline_sim.cpp  --  build: g++ -O2 -std=c++17 isa_pipeline_sim.cpp -o isa_sim
// usage: ./isa_sim program.asm [-f]      (-f = enable operand forwarding)
//
// 1) Functionally executes the assembly (ISA from the PDF) to get the dynamic instruction trace
//    and the real branch outcomes.
// 2) Feeds every executed instruction into a timing model of the 4-stage and 6-stage pipelines
//    (no cycle-by-cycle pipeline emulation) and reports stalls, flushes, CPI, time, speedup...
#include <bits/stdc++.h>
using namespace std;
typedef uint32_t u32; typedef int32_t i32; typedef long long ll;

// ---------------------------------------------------------------- ISA description
enum K{R3,CMP,INPL,R2,BIT,BR,RET,LD,ST,MOV,PUSH,POP,PEEK,MPOP,CLR,F3,FCMP,V3,VDOT,VMAG,NOP};
static const map<string,K> KIND={
 {"add",R3},{"sub",R3},{"mul",R3},{"div",R3},{"mod",R3},{"pow",R3},{"and",R3},{"or",R3},{"nor",R3},
 {"nand",R3},{"xor",R3},{"lsl",R3},{"lsr",R3},{"asr",R3},{"rotl",R3},{"rotr",R3},
 {"cmp",CMP},{"incr",INPL},{"decr",INPL},{"abs",INPL},{"not",INPL},{"sqrt",INPL},{"fabs",INPL},
 {"sin",INPL},{"cos",INPL},{"pcnt",R2},{"tog",BIT},{"sbit",BIT},{"cbit",BIT},
 {"call",BR},{"b",BR},{"beq",BR},{"bgt",BR},{"bst",BR},{"ret",RET},
 {"ld",LD},{"vld",LD},{"st",ST},{"vst",ST},{"mov",MOV},{"movu",MOV},{"movh",MOV},
 {"push",PUSH},{"pop",POP},{"peek",PEEK},{"mpop",MPOP},{"clr",CLR},
 {"fadd",F3},{"fsub",F3},{"fmul",F3},{"fdiv",F3},{"fcmp",FCMP},
 {"vadd",V3},{"vsub",V3},{"vcrs",V3},{"vdot",VDOT},{"vang",VDOT},{"vmag",VMAG},{"nop",NOP}};

struct Ins{ string op; K k=NOP; int a=-1,b=-1,c=-1; bool imm=false; ll iv=0; string lbl; size_t tgt=0;
            u32 src=0,dst=0; bool load=false; };

static const u32 STACK_TOP=0x100000;
static u32 bit(int r){return r>=0?1u<<r:0;}
static u32 vm(int g){return 7u<<(20+3*g);}                 // vector group g -> r(20+3g..22+3g)
static bool isReg(const string&t){return t=="sp"||t=="lr"||t=="pc"||(t.size()>1&&t[0]=='r'&&isdigit(t[1]));}
static bool isVec(const string&t){return t.size()==2&&t[0]=='v'&&t[1]>='0'&&t[1]<='2';}
static int rn(const string&t){ if(t=="sp")return 30; if(t=="lr")return 31; if(t=="pc")return 29;
    if(!isReg(t)||stoi(t.substr(1))>31) throw runtime_error("bad register '"+t+"'"); return stoi(t.substr(1)); }
static int vn(const string&t){ if(!isVec(t)) throw runtime_error("bad vector group '"+t+"'"); return t[1]-'0'; }
static float F(u32 x){float f;memcpy(&f,&x,4);return f;}
static u32 U(float f){u32 x;memcpy(&x,&f,4);return x;}

// ---------------------------------------------------------------- parser
static vector<Ins> parse(istream&in, map<string,size_t>&labels){
    vector<Ins> prog; string line; int ln=0;
    while(getline(in,line)){
        ln++;
        for(string cm:{";","//","@"}){ size_t p=line.find(cm); if(p!=string::npos) line.erase(p); }
        for(char&ch:line){ ch=tolower(ch); if(ch==','||ch=='['||ch==']'||ch=='+'||ch=='#') ch=' '; }
        stringstream ss(line); vector<string> t; string w; while(ss>>w) t.push_back(w);
        while(!t.empty() && t[0].back()==':'){ labels[t[0].substr(0,t[0].size()-1)]=prog.size(); t.erase(t.begin()); }
        if(t.empty()) continue;
        try{
            Ins I; I.op=t[0]; auto it=KIND.find(I.op);
            if(it==KIND.end()) throw runtime_error("unknown mnemonic '"+I.op+"'");
            I.k=it->second; t.erase(t.begin());
            auto T=[&](size_t i)->string{ if(i>=t.size()) throw runtime_error("missing operand"); return t[i]; };
            auto setImm=[&](const string&s){ I.imm=true;
                if(s.find('.')!=string::npos){ float f=stof(s); u32 b=U(f); I.iv=b; } else I.iv=stoll(s,0,0); };
            switch(I.k){
            case R3: I.a=rn(T(0)); I.b=rn(T(1)); if(isReg(T(2))) I.c=rn(T(2)); else setImm(T(2));
                I.dst=bit(I.a); I.src=bit(I.b)|bit(I.c); break;
            case CMP: I.b=rn(T(0)); if(isReg(T(1))) I.c=rn(T(1)); else setImm(T(1)); I.src=bit(I.b)|bit(I.c); break;
            case INPL: I.a=rn(T(0)); I.dst=I.src=bit(I.a); break;
            case R2: I.a=rn(T(0)); I.b=rn(T(1)); I.dst=bit(I.a); I.src=bit(I.b); break;
            case BIT: I.a=rn(T(0)); I.b=rn(T(1)); setImm(T(2)); I.dst=bit(I.a); I.src=bit(I.b); break;
            case BR: I.lbl=T(0); if(I.op=="call") I.dst=bit(31); break;
            case RET: I.src=bit(31); break;
            case LD: I.b=rn(T(1)); setImm(T(2)); I.src=bit(I.b);
                if(I.op=="vld"){ I.a=vn(T(0)); I.dst=vm(I.a);} else { I.a=rn(T(0)); I.dst=bit(I.a);} I.load=true; break;
            case ST: I.b=rn(T(1)); setImm(T(2));          // st rs1(value), [rd(base)+off]
                if(I.op=="vst"){ I.a=vn(T(0)); I.src=vm(I.a)|bit(I.b);} else { I.a=rn(T(0)); I.src=bit(I.a)|bit(I.b);} break;
            case MOV: I.a=rn(T(0)); I.dst=bit(I.a);
                if(isReg(T(1))){ I.c=rn(T(1)); I.src=bit(I.c);} else setImm(T(1));
                if(I.op=="movh") I.src|=bit(I.a); break;
            case PUSH: I.a=rn(T(0)); I.src=bit(I.a)|bit(30); I.dst=bit(30); break;
            case POP: I.a=rn(T(0)); I.src=bit(30); I.dst=bit(I.a)|bit(30); break;
            case PEEK: I.a=rn(T(0)); I.src=bit(30); I.dst=bit(I.a); break;
            case MPOP: setImm(T(0)); I.src=I.dst=bit(30); break;
            case CLR: I.dst=bit(30); break;
            case F3: I.a=rn(T(0)); I.b=rn(T(1)); I.c=rn(T(2)); I.dst=bit(I.a); I.src=bit(I.b)|bit(I.c); break;
            case FCMP: I.b=rn(T(0)); I.c=rn(T(1)); I.src=bit(I.b)|bit(I.c); break;
            case V3: I.a=vn(T(0)); I.b=vn(T(1)); I.c=vn(T(2)); I.dst=vm(I.a); I.src=vm(I.b)|vm(I.c); break;
            case VDOT: I.a=rn(T(0)); I.b=vn(T(1)); I.c=vn(T(2)); I.dst=bit(I.a); I.src=vm(I.b)|vm(I.c); break;
            case VMAG: I.a=rn(T(0)); I.b=vn(T(1)); I.c=I.b; I.dst=bit(I.a); I.src=vm(I.b); break;
            case NOP: break;
            }
            I.dst&=~bit(29);                               // r29 (PC) is not a normal destination
            prog.push_back(I);
        }catch(exception&e){ fprintf(stderr,"line %d: %s\n",ln,e.what()); exit(1); }
    }
    for(size_t i=0;i<prog.size();i++){ Ins&I=prog[i];     // resolve branch targets (label or numeric offset)
        if(I.k!=BR) continue;
        if(labels.count(I.lbl)) I.tgt=labels[I.lbl];
        else try{ I.tgt=i+stoll(I.lbl,0,0); }catch(...){ fprintf(stderr,"unknown label '%s'\n",I.lbl.c_str()); exit(1);} }
    return prog;
}

// ---------------------------------------------------------------- functional execution
struct CPU{ u32 R[32]={0}; bool E=0,GT=0,ST=0; unordered_map<i32,u32> mem; size_t pc=0; };

// returns control type: 0 none, 1 cond. branch not taken, 2 cond. taken, 3 unconditional jump (b/call/ret)
static int exec(CPU&c,const Ins&I){
    u32*R=c.R; size_t next=c.pc+1; int ctrl=0; const string&o=I.op;
    auto B=[&]{return I.imm?(u32)I.iv:R[I.c];};
    switch(I.k){
    case R3:{ u32 ux=R[I.b], uy=B(); i32 x=ux, y=uy; u32 r=0;
        if(o=="add")r=ux+uy; else if(o=="sub")r=ux-uy; else if(o=="mul")r=ux*uy;
        else if(o=="div")r=y?(u32)((ll)x/y):0; else if(o=="mod")r=y?(u32)((ll)x%y):0;
        else if(o=="pow"){r=1;for(u32 i=0;i<uy&&i<64;i++)r*=ux;}
        else if(o=="and")r=ux&uy; else if(o=="or")r=ux|uy; else if(o=="nor")r=~(ux|uy);
        else if(o=="nand")r=~(ux&uy); else if(o=="xor")r=ux^uy; else if(o=="lsl")r=ux<<(uy&31);
        else if(o=="lsr")r=ux>>(uy&31); else if(o=="asr")r=(u32)(x>>(uy&31));
        else{ u32 s=uy&31; if(o=="rotl")r=s?(ux<<s)|(ux>>(32-s)):ux; else r=s?(ux>>s)|(ux<<(32-s)):ux; }
        R[I.a]=r; break; }
    case CMP:{ i32 x=R[I.b], y=B(); c.E=x==y; c.GT=x>y; c.ST=x<y; break; }
    case INPL: if(o=="incr")R[I.a]++; else if(o=="decr")R[I.a]--; else if(o=="abs")R[I.a]=abs((i32)R[I.a]);
        else if(o=="not")R[I.a]=~R[I.a]; else if(o=="sqrt")R[I.a]=U(sqrtf(F(R[I.a])));
        else if(o=="fabs")R[I.a]=U(fabsf(F(R[I.a]))); else if(o=="sin")R[I.a]=U(sinf(F(R[I.a])));
        else R[I.a]=U(cosf(F(R[I.a]))); break;
    case R2: R[I.a]=__builtin_popcount(R[I.b]); break;
    case BIT:{ u32 m=1u<<(I.iv&31); R[I.a]= o=="tog"?R[I.b]^m : o=="sbit"?R[I.b]|m : R[I.b]&~m; break; }
    case BR:
        if(o=="b"){ctrl=3;next=I.tgt;}
        else if(o=="call"){R[31]=c.pc+1;ctrl=3;next=I.tgt;}
        else{ bool t=o=="beq"?c.E:o=="bgt"?c.GT:c.ST; ctrl=t?2:1; if(t)next=I.tgt; }
        break;
    case RET: ctrl=3; next=R[31]; break;
    case LD:{ i32 ad=(i32)R[I.b]+(i32)I.iv;
        if(o=="ld")R[I.a]=c.mem[ad]; else for(int j=0;j<3;j++)R[20+3*I.a+j]=c.mem[ad+j]; break; }
    case ST:{ i32 ad=(i32)R[I.b]+(i32)I.iv;
        if(o=="st")c.mem[ad]=R[I.a]; else for(int j=0;j<3;j++)c.mem[ad+j]=R[20+3*I.a+j]; break; }
    case MOV: if(o=="movh")R[I.a]=(R[I.a]&0xFFFF)|((u32)I.iv<<16); else R[I.a]=I.imm?(u32)I.iv:R[I.c]; break;
    case PUSH: R[30]--; c.mem[R[30]]=R[I.a]; break;
    case POP: R[I.a]=c.mem[R[30]]; R[30]++; break;
    case PEEK: R[I.a]=c.mem[R[30]]; break;
    case MPOP: R[30]+=I.iv; break;
    case CLR: R[30]=STACK_TOP; break;
    case F3:{ float x=F(R[I.b]),y=F(R[I.c]),r= o=="fadd"?x+y: o=="fsub"?x-y: o=="fmul"?x*y: x/y; R[I.a]=U(r); break; }
    case FCMP:{ float x=F(R[I.b]),y=F(R[I.c]); c.E=x==y; c.GT=x>y; c.ST=x<y; break; }
    case V3:{ float u[3],v[3],r[3]; for(int j=0;j<3;j++){u[j]=F(R[20+3*I.b+j]);v[j]=F(R[20+3*I.c+j]);}
        if(o=="vadd")for(int j=0;j<3;j++)r[j]=u[j]+v[j]; else if(o=="vsub")for(int j=0;j<3;j++)r[j]=u[j]-v[j];
        else{r[0]=u[1]*v[2]-u[2]*v[1];r[1]=u[2]*v[0]-u[0]*v[2];r[2]=u[0]*v[1]-u[1]*v[0];}
        for(int j=0;j<3;j++)R[20+3*I.a+j]=U(r[j]); break; }
    case VDOT: case VMAG:{ float d=0,nu=0,nv=0;
        for(int j=0;j<3;j++){float u=F(R[20+3*I.b+j]),v=F(R[20+3*I.c+j]);d+=u*v;nu+=u*u;nv+=v*v;}
        R[I.a]= o=="vdot"?U(d) : o=="vmag"?U(sqrtf(nu)) : U(acosf(d/(sqrtf(nu)*sqrtf(nv)))); break; }
    case NOP: break;
    }
    c.pc=next; return ctrl;
}

// ---------------------------------------------------------------- pipeline timing model
// Each executed instruction gets an entry cycle e[s] in every stage s (in-order, stalls hold earlier stages).
//  * RAW hazard, no forwarding : operand-read stage waits until producer's writeback stage (reg file written in 1st half-cycle)
//  * RAW hazard, forwarding    : EX waits until producer's EX result (ALU) or MA result (load) is ready
//  * Branches: static predict-not-taken, resolved in EX. Taken branch / jump / call / ret => flush of the
//    instructions fetched in the stages before EX, fetch restarts the cycle after EX.
struct Pipe{
    string name; vector<double> d; int n,R,EX,MA,W; bool fwd;
    vector<ll> prev; ll redirect=0, ready[32]={0};
    ll N=0,stalls=0,flushes=0,flushedInstr=0,flushCycles=0,branches=0,mispred=0,last=0;
    Pipe(string nm,vector<double> dl,int r,int ex,int ma,int w,bool f):name(nm),d(dl),n(dl.size()),R(r),EX(ex),MA(ma),W(w),fwd(f),prev(dl.size(),-1){}
    double period()const{return *max_element(d.begin(),d.end());}
    void feed(const Ins&I,int ctrl){
        vector<ll> e(n); int hs=fwd?EX:R;
        for(int s=0;s<n;s++){
            ll t= s? e[s-1]+1 : redirect;
            t=max(t, s<n-1? prev[s+1] : prev[s]+1);            // wait until next stage is free
            if(s==hs){ ll h=0; for(int r=0;r<32;r++) if(I.src>>r&1) h=max(h,ready[r]);
                       if(h>t){stalls+=h-t;t=h;} }               // data-hazard stall cycles
            e[s]=t;
        }
        redirect=0;
        for(int r=0;r<32;r++) if(I.dst>>r&1) ready[r]= fwd ? e[I.load?MA:EX]+1 : e[W];
        if(ctrl){ branches++;
            if(ctrl>=2){ flushes++; flushedInstr+=EX; redirect=e[EX]+1; flushCycles+=max(0LL,redirect-e[1]); }
            if(ctrl==2) mispred++; }
        prev=e; last=e[n-1]; N++;
    }
    ll cycles()const{return last+1;}
};

int main(int argc,char**argv){
    if(argc<2){ fprintf(stderr,"usage: %s program.asm [-f]\n",argv[0]); return 1; }
    bool fwd=argc>2&&string(argv[2])=="-f";
    ifstream f(argv[1]); if(!f){ fprintf(stderr,"cannot open %s\n",argv[1]); return 1; }
    map<string,size_t> labels; vector<Ins> prog=parse(f,labels);

    // name, stage delays (ns), {operand-read stage, EX stage, MA stage, writeback stage}
    vector<Pipe> P;
    P.emplace_back("4-stage",vector<double>{3.0,3.0,3.0,4.0},       1,2,3,3,fwd);            // IF OF EX MA+RW
    P.emplace_back("6-stage",vector<double>{3.0,1.5,1.5,3.0,3.0,1.0},2,3,4,5,fwd);           // IF ID RR EX MA RW

    CPU cpu; cpu.R[30]=STACK_TOP; cpu.R[31]=prog.size();     // top-level 'ret' ends the program
    const ll LIMIT=5000000; ll cnt=0;
    while(cpu.pc<prog.size() && cnt<LIMIT){ const Ins&I=prog[cpu.pc]; int ctrl=exec(cpu,I); for(auto&p:P)p.feed(I,ctrl); cnt++; }
    if(cnt>=LIMIT) fprintf(stderr,"warning: stopped after %lld instructions (possible infinite loop)\n",LIMIT);
    if(!cnt){ puts("no instructions executed"); return 0; }

    printf("Forwarding: %s | predictor: static not-taken, branch resolved in EX\n\n",fwd?"ON":"OFF");
    printf("%-26s %14s %14s\n","Metric",P[0].name.c_str(),P[1].name.c_str());
    auto row=[&](const char*m,function<double(Pipe&)> g,const char*fmt){ char a[64],b[64];
        snprintf(a,64,fmt,g(P[0])); snprintf(b,64,fmt,g(P[1])); printf("%-26s %14s %14s\n",m,a,b); };
    row("Total instructions",[](Pipe&p){return (double)p.N;},"%.0f");
    row("Clock cycles",[](Pipe&p){return (double)p.cycles();},"%.0f");
    row("Clock period (ns)",[](Pipe&p){return p.period();},"%.2f");
    row("Stall cycles (data)",[](Pipe&p){return (double)p.stalls;},"%.0f");
    row("Flushes (events)",[](Pipe&p){return (double)p.flushes;},"%.0f");
    row("Flushed instr. slots",[](Pipe&p){return (double)p.flushedInstr;},"%.0f");
    row("Flush penalty cycles",[](Pipe&p){return (double)p.flushCycles;},"%.0f");
    row("Branches/jumps executed",[](Pipe&p){return (double)p.branches;},"%.0f");
    row("Branch mispredictions",[](Pipe&p){return (double)p.mispred;},"%.0f");
    row("CPI",[](Pipe&p){return (double)p.cycles()/p.N;},"%.3f");
    row("Execution time (ns)",[](Pipe&p){return p.cycles()*p.period();},"%.1f");
    double t4=P[0].cycles()*P[0].period(), t6=P[1].cycles()*P[1].period();
    printf("\nSpeedup (6-stage over 4-stage) = T4/T6 = %.3f\n",t4/t6);
    printf("%s is faster by %.1f%%\n",t6<t4?"6-stage":"4-stage",fabs(t4-t6)/max(t4,t6)*100);
}
