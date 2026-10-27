#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

struct Config { string instance, outputDir = "results"; int customers = 0, uavs = 0, population = 100, generations = 1000; unsigned seed = 1; double emax = 0, energyRate = 1, mutationRate = .2, eliteFraction = .1; };
struct Individual { vector<int> chromosome; double distance = numeric_limits<double>::infinity(), maxEnergy = numeric_limits<double>::infinity(), violation = numeric_limits<double>::infinity(), score = numeric_limits<double>::infinity(); };
int n, m; double emax, energyRate; vector<pair<double,double>> points; vector<vector<double>> d;

void usage(const char* p) { cerr << "Usage: " << p << " --instance FILE --customers N --uavs K --emax VALUE --seed S [--output-dir DIR] [--population P] [--generations G]\n"; }
Config parse(int argc, char** argv) {
    Config c;
    for (int i=1;i<argc;++i) { string k=argv[i]; if(k=="--help"){usage(argv[0]);exit(0);} if(i+1==argc) throw runtime_error("Missing value for "+k); string v=argv[++i];
        if(k=="--instance")c.instance=v; else if(k=="--customers")c.customers=stoi(v); else if(k=="--uavs")c.uavs=stoi(v); else if(k=="--emax")c.emax=stod(v); else if(k=="--seed")c.seed=stoul(v); else if(k=="--output-dir")c.outputDir=v; else if(k=="--population")c.population=stoi(v); else if(k=="--generations")c.generations=stoi(v); else if(k=="--energy-rate")c.energyRate=stod(v); else if(k=="--mutation-rate")c.mutationRate=stod(v); else if(k=="--elite-fraction")c.eliteFraction=stod(v); else throw runtime_error("Unknown option: "+k); }
    if(c.instance.empty()||c.customers<=0||c.uavs<=0||c.emax<=0||c.population<4||c.generations<=0||c.energyRate<=0||c.mutationRate<0||c.mutationRate>1||c.eliteFraction<=0||c.eliteFraction>=1) throw runtime_error("Invalid or incomplete arguments. Use --help."); return c;
}
void loadInstance(const string& path, int requested) {
    ifstream in(path); if(!in) throw runtime_error("Cannot open instance: "+path); vector<pair<double,double>> all; string line;
    while(getline(in,line)){ istringstream s(line); int id; double x,y,a,b,c,e; if(s>>id>>x>>y>>a>>b>>c>>e && id>=0) { if((int)all.size()<=id) all.resize(id+1); all[id]={x,y}; } }
    if((int)all.size()<=requested) throw runtime_error("Instance does not contain requested customers."); n=requested; points.assign(all.begin(),all.begin()+n+1); d.assign(n+1,vector<double>(n+1));
    for(int i=0;i<=n;++i)for(int j=0;j<=n;++j)d[i][j]=hypot(points[i].first-points[j].first,points[i].second-points[j].second);
}
void evaluate(Individual& x) {
    double total=0, route=0, longest=0; int previous=0;
    for(int node:x.chromosome){ total+=d[previous][node]; route+=d[previous][node]; if(node==0){longest=max(longest,route);route=0;} previous=node; }
    total+=d[previous][0]; route+=d[previous][0]; longest=max(longest,route); x.distance=total; x.maxEnergy=energyRate*longest; x.violation=max(0.0,x.maxEnergy-emax); x.score=x.distance+(x.violation>0?1000000+10000*x.violation:0);
}
Individual randomIndividual(mt19937& rng) { Individual x; for(int i=1;i<=n;++i)x.chromosome.push_back(i); for(int i=1;i<m;++i)x.chromosome.push_back(0); shuffle(x.chromosome.begin(),x.chromosome.end(),rng); evaluate(x); return x; }
Individual crossover(const Individual& a,const Individual& b,mt19937& rng) {
    int len=(int)a.chromosome.size(); uniform_int_distribution<int> pick(0,len-1); int left=pick(rng),right=pick(rng); if(left>right)swap(left,right); Individual c; c.chromosome.assign(len,-1); vector<int> remaining(n+1);
    for(int v:a.chromosome)++remaining[v]; for(int i=left;i<=right;++i){c.chromosome[i]=a.chromosome[i];--remaining[a.chromosome[i]];} int put=(right+1)%len;
    for(int off=0;off<len;++off){int v=b.chromosome[(right+1+off)%len];if(!remaining[v])continue;while(c.chromosome[put]!=-1)put=(put+1)%len;c.chromosome[put]=v;--remaining[v];} evaluate(c);return c;
}
void mutate(Individual& x,mt19937& rng){uniform_int_distribution<int>pick(0,(int)x.chromosome.size()-1);swap(x.chromosome[pick(rng)],x.chromosome[pick(rng)]);evaluate(x);}
const Individual& tournament(const vector<Individual>& p,mt19937& rng){uniform_int_distribution<int>pick(0,(int)p.size()-1);const Individual&a=p[pick(rng)],&b=p[pick(rng)];return a.score<b.score?a:b;}
void writeFiles(const Config& c,const vector<double>& curve,const Individual& best,double seconds) {
    string prefix=c.outputDir+"/run_seed_"+to_string(c.seed); ofstream out(prefix+"_convergence.csv");out<<"generation,best_objective\n"<<fixed<<setprecision(6);for(size_t i=0;i<curve.size();++i)out<<i+1<<','<<curve[i]<<'\n';
    string instanceName=c.instance; size_t slash=instanceName.find_last_of("\\/"); if(slash!=string::npos) instanceName=instanceName.substr(slash+1); size_t dot=instanceName.find_last_of('.'); if(dot!=string::npos) instanceName=instanceName.substr(0,dot);
    ofstream summary(prefix+"_summary.csv");summary<<"instance,customers,uavs,emax,energy_rate,seed,population,generations,best_objective,max_route_energy,feasible,runtime_seconds\n";summary<<instanceName<<','<<n<<','<<m<<','<<fixed<<setprecision(6)<<emax<<','<<energyRate<<','<<c.seed<<','<<c.population<<','<<c.generations<<','<<best.distance<<','<<best.maxEnergy<<','<<(best.violation<=1e-9?1:0)<<','<<seconds<<'\n';
    ofstream route(prefix+"_route.txt");route<<"seed="<<c.seed<<"\nobjective="<<best.distance<<"\nmax_route_energy="<<best.maxEnergy<<"\n";int uav=1;route<<"UAV "<<uav<<": 0";for(int v:best.chromosome){if(v==0){route<<" -> 0\n";if(uav<m)route<<"UAV "<<++uav<<": 0";}else route<<" -> "<<v;}route<<" -> 0\n";
}
int main(int argc,char** argv){try{Config c=parse(argc,argv);m=c.uavs;emax=c.emax;energyRate=c.energyRate;loadInstance(c.instance,c.customers);mt19937 rng(c.seed);auto started=chrono::steady_clock::now();vector<Individual> pop(c.population);for(auto&x:pop)x=randomIndividual(rng);Individual best=pop[0];vector<double> curve;bernoulli_distribution shouldMutate(c.mutationRate);int elites=max(1,(int)round(c.eliteFraction*c.population));
    for(int g=0;g<c.generations;++g){sort(pop.begin(),pop.end(),[](const Individual&a,const Individual&b){return a.score<b.score;});if(pop[0].score<best.score)best=pop[0];curve.push_back(best.violation<=1e-9?best.distance:numeric_limits<double>::quiet_NaN());vector<Individual>next(pop.begin(),pop.begin()+elites);while((int)next.size()<c.population){Individual child=crossover(tournament(pop,rng),tournament(pop,rng),rng);if(shouldMutate(rng))mutate(child,rng);next.push_back(child);}pop.swap(next);}sort(pop.begin(),pop.end(),[](const Individual&a,const Individual&b){return a.score<b.score;});if(pop[0].score<best.score)best=pop[0];double seconds=chrono::duration<double>(chrono::steady_clock::now()-started).count();writeFiles(c,curve,best,seconds);cout<<fixed<<setprecision(6)<<"objective="<<best.distance<<" feasible="<<(best.violation<=1e-9)<<" runtime_seconds="<<seconds<<'\n';}
    catch(const exception&e){cerr<<"Error: "<<e.what()<<'\n';return 1;}}
