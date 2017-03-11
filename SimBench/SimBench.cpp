#include "Globals.h"
#include "Allocator.h"
#include "Hash.h"
#include "Gates.h"
#include "UniformGenerator.h"
#include "lfsr.h"
#include "Draw.h"


// Alokator polaczen i bramek
Allocator<int> Alloc(1000);
Allocator<SGate> GAlloc(30000);

// Tablica polaczen
typedef std::unordered_map<unsigned int,SGate*> CTable;
CTable connection_table;

struct SError
{
	std::string name;	// Nazwa do DEBUGOWANIA
	SGate* gate;		// Bramka ktorej blad dotyczy
	int InputIndex;		// Index wejscia
	int DamageValue;	// Wartosc bledu
	bool DamageType;	// 0 - Output, 1 - Input
	SError() : gate(NULL),InputIndex(0),DamageValue(0),DamageType(false) {}
	void SetError()
	{
		gate->Damaged = true;
		gate->DamageType = DamageType;
		gate->DamageValue = DamageValue;
		gate->DamageIndex = InputIndex;
	}
	void DisableError()
	{
		gate->Damaged = false;
		// Potrzebne ?
		gate->DamageType = 0;
		gate->DamageValue = 0;
		gate->DamageIndex = 0;
	}
};

std::vector<SError *> errors;	// Lista wszystkich bledow

std::vector<SGate*> gates;		// Wszystkie bramki
std::vector<SGate*> inputs;		// Wszystkie wejscia
std::vector<SGate*> outputs;	// Wszystkie wyjscia
std::vector<SGate*> dff_gates;	// Bramki DFF

int		count_inputs = 0;
int		count_outputs = 0;
int		count_gates = 0;
int		count_errors = 0;

//SGate** arr = NULL; wolniejsze o 1sek dla s38584 :/
SGate* arr[30000];	// Statyczna wartosc jak zmienic bez tego co wy¿ej ? :(
int arr_size = 0;

class SimBench
{
	CTable::iterator conn_it;
	FILE*	pFile;
	long	filesize;
	char	line[100];
public:
	SimBench() : pFile(NULL), filesize(0) {}
	~SimBench() { SAFE_FILECLOSE(pFile); }
public:
	int GetGateType(char* gate) // Do poprawy
	{
		if(gate[0] == 'A') // A co jeœli pojawi siê bramka 'A'BCDE...?
		{
			return G_AND;
		}
		else if(gate[0] == 'D')
		{
			return G_DFF;
		}
		else if(gate[0] == 'O')
		{
			return G_OR;
		}
		else if(gate[0] == 'N')
		{
			if(gate[1] == 'A')
			{
				return G_NAND;
			}
			else if(gate[1] == 'O')
			{
				return (gate[2] == 'R') ? G_NOR : G_NOT;
			}
		}
		else if(gate[0] == 'X')
		{
			return (gate[1] == 'O') ? G_XOR : G_XNOR;
		}
		return G_NONE;
	}
	bool ParseBench(char* filename)
	{
		FILE*		 pFile;
		char*		 pch;
		int			 pos;
		unsigned int hash_val;
		std::string file;
		file.append(filename);
		file.append(".bench");
		//=================================================================================
		pFile = fopen(file.c_str(),"rb");
		if(pFile != NULL)
		{
			printf("BENCH File: %s ",file.c_str());
			while (fgets(line,100,pFile) != NULL)
			{		
				pos = 0;
				if(strchr(line,'=') != NULL)
				{
					SGate *gate;	// Bramka dodawana
					SGate *in_gate;	// Bramka wejsciowa
					int *input[8];

					// BRAMKA - WYJSCIE
					pch = strtok (line,Skip);
					hash_val = fast_hash(pch);
					conn_it = connection_table.find(hash_val);
					if(conn_it == connection_table.end())
					{
						// Nie znalezione
						gate = GAlloc.Add();
						gate->name.append(pch);
						gate->Output = Alloc.Add();
						gate->gate_level = -1;
						connection_table.insert(CTable::value_type(hash_val,gate));
						gates.push_back(gate);	
						count_gates++;
					}
					else // Znalezione
					{
						gate = conn_it->second;
					}

					// BRAMKA - TYP
					pch = strtok (NULL,Skip);
					gate->GateType = GetGateType(pch);
					gate->SetDelegator();

					if(gate->GateType == G_DFF) dff_gates.push_back(gate);
					
					// BRAMKA - WEJSCIA
					pch = strtok (NULL,Skip);
					while (pch != NULL)
					{
						hash_val = fast_hash(pch);
						conn_it = connection_table.find(hash_val);

						if(conn_it == connection_table.end())
						{
							in_gate = GAlloc.Add();
							in_gate->gate_level = -1;
							in_gate->name.append(pch);
							in_gate->Output = Alloc.Add();
							in_gate->connection.push_back(gate);
							connection_table.insert(CTable::value_type(hash_val,in_gate));
							gates.push_back(in_gate);
							count_gates++;
						}
						else
						{
							// Znalezione
							in_gate = conn_it->second;
							in_gate->connection.push_back(gate);
						}
						input[pos] = in_gate->Output;
						pos++;
						pch = strtok (NULL,Skip);
					}
					gate->InputCount = pos;
					for(int x = 0; x < pos; x++)
						gate->Input[x] = input[x];
				}
				else
				{
					if(!strncmp(line,"INPUT",5)) 
					{
						pch = strtok (line,Skip);
						while( pch != NULL )
						{
							pch = strtok (NULL,Skip);
							pos++;
							if(pos == 1)
							{
								hash_val = fast_hash(pch);
								if(connection_table.find(hash_val) == connection_table.end())	// nie znalaz³
								{
									SGate *gate = GAlloc.Add();
									gate->gate_level = -1;
									gate->GateType = G_INPUT;
									gate->SetDelegator();
									gate->name.append(pch);
									gate->Output = Alloc.Add();
									gate->Input[0] = Alloc.Add();
									connection_table.insert(CTable::value_type(hash_val,gate));
									gates.push_back(gate);
									inputs.push_back(gate);
									count_inputs++;
								}
								// TODO: Dodaæ obs³ugê jak istnieje wejœcie
							}
						}
					}
					else if(!strncmp(line,"OUTPUT",6))
					{
						SGate *out_gate = GAlloc.Add();
						out_gate->gate_level = -1;

						pch = strtok (line,Skip);
						while( pch != NULL )
						{
							pch = strtok (NULL,Skip);
							pos++;
							if(pos == 1)
							{
								// Tworzymy bramke wyjsciowa...
								SGate *out_gate = GAlloc.Add();
								out_gate->gate_level = -1;
								out_gate->GateType = G_OUTPUT;
								out_gate->SetDelegator();
								out_gate->name.append("OUT");
								out_gate->name.append(pch);
								hash_val = fast_hash(pch);

								if(connection_table.find(hash_val) == connection_table.end()) // Jesli wejscie nie istnieje to dodajemy je :)
								{
									SGate *gate = GAlloc.Add();
									gate->gate_level = -1;
									gate->name.append(pch);
									gate->Output = Alloc.Add();
									gate->connection.push_back(out_gate);
									out_gate->Output = gate->Output;
									connection_table.insert(CTable::value_type(hash_val,gate));
									gates.push_back(gate);
									count_gates++;
									count_outputs++;
								}
								gates.push_back(out_gate);
								outputs.push_back(out_gate);
							}
						}
					}
				}
			}
			printf("[ IN: %i OUT: %i GATES: %i ]\n",count_inputs,count_outputs,count_gates);
			SAFE_FILECLOSE(pFile);
			return true;
		}
		return false;
	}
	bool ParseFLT(char*filename)
	{
		FILE*		 pFile = NULL;
		char*		 pch = NULL;
		int			 posline = 0;
		unsigned int hash_val = 0;
		std::string file;
		//=================================================================================
		file.append(filename);
		file.append(".flt");
		pFile = fopen(file.c_str(),"rb");
		if(pFile != NULL)
		{
			printf("FLT   File: %s ",file.c_str());
			while (fgets(line,100,pFile) != NULL)
			{
				posline++;
				if(line[0] != '#')
				{
					if(strchr(line,'->') == NULL) // OUTPUT DAMAGE
					{
						pch = strtok(line,SkipError);
						SGate *DamagedGate = NULL;
						hash_val = fast_hash(pch);
						conn_it = connection_table.find(hash_val);

						if(conn_it != connection_table.end()) DamagedGate = conn_it->second;
						// DEBUG!!
						if(DamagedGate == NULL) printf("DamagedGate return NULL -> %s LINE %i\n",pch,posline);

						pch = strtok (NULL,Skip);
						while (pch != NULL)
						{
							SError *ErrorInfo = new SError();
							ErrorInfo->name.append(line);
							ErrorInfo->DamageType = 0;
							ErrorInfo->InputIndex = 0;
							ErrorInfo->gate = DamagedGate;
							ErrorInfo->DamageValue = (pch[1] == '0') ? 0 : 1;
							errors.push_back(ErrorInfo);
							pch = strtok (NULL,Skip);
						}
					}
					else // INPUT DAMAGE
					{
						std::string name;
						SGate *InputGate = NULL;
						SGate *DamagedGate = NULL;
						int index = 0;
						pch = strtok(line,SkipError);
						name.append(pch);
						name.append("->");
					
						// Bramka [GXX] -> Gyy
						hash_val = fast_hash(pch);
						conn_it = connection_table.find(hash_val);
						if(conn_it != connection_table.end()) InputGate = conn_it->second;

						// Bramka Gxx -> [GYY]
						pch = strtok(NULL,SkipError);
						name.append(pch);
						hash_val = fast_hash(pch);
						conn_it = connection_table.find(hash_val);
						if(conn_it != connection_table.end()) DamagedGate = conn_it->second;

						// DEBUG!!
						if(DamagedGate == NULL)
							printf("DamagedGate return NULL -> %s\n",pch);

						for(int i = 0; i < DamagedGate->InputCount; i++)
						{
							if(DamagedGate->Input[i] == InputGate->Output) index = i;// Porownujemy adresy jesli sie zgadzaja to mamy Index wejscia
						}
					
						pch=strtok(NULL,Skip);
					
						while (pch != NULL)
						{
							SError *ErrorInfo = new SError();
							ErrorInfo->name.append(name);
							ErrorInfo->DamageType = 1;
							ErrorInfo->InputIndex = index;
							ErrorInfo->gate = DamagedGate;
							ErrorInfo->DamageValue = (pch[1] == '0') ? 0 : 1;
							errors.push_back(ErrorInfo);
							pch = strtok (NULL,Skip);
						}
					}
				}
			}
			/*
			for(int i = 0; i < errors.size(); i++)
				printf("ERROR NAME: %s VALUE %i INDEX %i\n",errors[i]->name.c_str(),errors[i]->DamageValue,errors[i]->InputIndex);
			*/

			printf("[ ERRORS FOUND: %i ]\n\n",errors.size());
			return true;
		}
		return false;

	}
	void Levelize()
	{
		// DFF's
		for(int x = 0; x< dff_gates.size();x++)
			dff_gates[x]->SetLevel(0);
		
		// INPUTS
		for(int x = 0; x< inputs.size();x++)
			inputs[x]->SetLevel(0);
	}
};

class GATESManager
{
	typedef struct SBlock
	{
		std::vector<SGate*> gatevec;
	} GBlock;
	int *leveltable;	// Tabela zawiera informacje ile jest bramek na danym poziomie
	int blockcount;		// Informajce ile jest poziomow
	int pos;
	//==============================
	UniformGenerator *Generator;
	GBlock * Block;
	int gRuns;
	std::vector<int*> input_table;
	std::vector<int*> output_table;
	float *graf;

public:
	GATESManager() : leveltable(NULL), Block(NULL), blockcount(0),pos(0) {}
	~GATESManager() {}
	void SetTable(int maxlvl)
	{
		blockcount = maxlvl + 1;
		leveltable = (int*) calloc(blockcount,4);
		
		Block = new GBlock[blockcount];

		arr_size = gates.size();
		arr_size -= inputs.size();
		arr_size -= dff_gates.size();
		//arr = new SGate*[arr_size];	// Dla dynamicznego rozmiaru

		/* Wypelniamy tablice ile jest elementow dla danego poziomu*/
		for( int x = 0; x < gates.size(); x++)
			(gates[x]->gate_level > -1) ? leveltable[gates[x]->gate_level]++ : printf("WARNING: !!! GATE %s NOT CONNECTED !!!\n",gates[x]->name.c_str());

		for( int x = 0; x < gates.size(); x++)
		{
			int lvl = gates[x]->gate_level;
			if(lvl>-1) Block[lvl].gatevec.push_back(gates[x]);
		}

		for( int x = 0; x < blockcount; x++)
		{
			for(int y = 0; y < Block[x].gatevec.size(); y++)
			{
				if(x>0)	// Zapisujemy wszystko poza DFF i INPUT
				{
					arr[pos] = Block[x].gatevec[y];
					pos++;
				}
			}
		}
	}
	void CreateErrorLog()
	{

	}
	void RunSampleTest(int runs)	//TEST wzorcowy
	{
		// Petla testujaca
		for(int x = 0; x < runs; x++)
		{
			// Ustaw wejscia
			for(int y = 0; y < count_inputs; y++)
			{
				*inputs[y]->Input[0] = input_table[x][y];
				inputs[y]->Update();
			}

			// Przeprowadz test
			for(int y = 0; y < arr_size; y++)
				arr[y]->Update();	// Szybko !! :D

			// Zapisz wyniki
			for(int y = 0; y < count_outputs; y++)
				output_table[x][y] = *outputs[y]->Output;
			
			// Zaktualizuj DFF'y
			for(int y = 0; y < dff_gates.size(); y++)
				dff_gates[y]->Update();
		}
	}
	void RunBenchmark(int runs, bool test_type, bool logtofile = false, bool showinfo = false) // 0 - przerywa przy bledzie, 1 - przebieg do konca 
	{
		SError * curError = NULL;		// poprzedni blad
		bool ErrorFound = false;
		int	 ErrorCount = 0;

		std::string line;
		char* pch = new char[10];
		std::ofstream file;

		if(runs>0) graf = (float*) calloc(runs,4);

		gRuns = runs;
		input_table.clear();
		output_table.clear();
		// Alokacja dla wynikow wzorcowych i wektorow
		for(int x = 0; x < runs; x++)
		{
			input_table.push_back((int*) calloc(count_inputs,4));
			output_table.push_back((int*) calloc(count_outputs,4));
		}
		int max_value = (int)pow((float)2,count_inputs);
		double vector;
		srand((unsigned int)time(NULL));
		Generator = new UniformGenerator(rand(),0,max_value);

		for(int x = 0; x < runs; x++) // Generowanie wektorow
		{
			vector = Generator->NextValue();
			for(int y = 0; y < count_inputs; y++)
			{
				input_table[x][y] = (((int)floor(vector) & (1 << (count_inputs-y-1))) > 0) ? 1 : 0; // poprawic !!!!
			}
		}

		if( logtofile )
		{
			
			file.open("vector.txt");
			for(int x = 0; x < runs; x++) // Generowanie wektorow
			{
				line.clear();
				for(int y = 0; y < count_inputs; y++)
				{
					itoa(input_table[x][y],pch,10);
					line.append(pch);
				}
				line.append("\n");
				file << line;		
			}
			file.close();
			line.clear();
		}

		RunSampleTest(runs);
		if(logtofile)
		{
			file.open("result.txt");
			file << "#pattern test\n\n";

			for(int x = 0; x < runs; x++) // Generowanie wektorow
			{
				line.clear();
				for(int y = 0; y < count_outputs; y++)
				{
					itoa(output_table[x][y],pch,10);
					line.append(pch);
				}
				file << line << "\n";
			}
		}
		// Uszkodzenia
		for(int x = 0; x < errors.size(); x++)
		{

			// Resetowanie DFF'ów
			for(int y = 0; y < dff_gates.size(); y++)
				dff_gates[y]->ResetDFF();

			// Usuwamy stare uszkodzenie i ustawiamy nowe
			if(x > 0 && curError != NULL) curError->DisableError();
			curError = errors[x];
			curError->SetError();

			if( logtofile ) file << "\n# Test " << errors[x]->name << "\n\n";

			// Testujemy
			for(int z = 0; z < runs; z++)
			{
				ErrorFound = false;
				// Ustawienie wejsc
				for(int y = 0; y < count_inputs; y++)
				{
					*inputs[y]->Input[0] = input_table[z][y];
					inputs[y]->Update();
				}

				// Aktualizacja bramek
				for(int y = 0; y < arr_size; y++)
					arr[y]->Update();	// Szybko !! :D

				// Porownanie wyniku
				for(int y = 0; y < count_outputs; y++)
				{
					if(logtofile)
					{
						line.clear();
						itoa(output_table[z][y],pch,10);
						line.append(pch);
						file << line;
					}

					if(output_table[z][y] != *outputs[y]->Output)
					{
						for(int i = z; i < runs; i++)
						{
							graf[i]++;
						}
						ErrorFound = true;
						break;
					}
				}

				if(logtofile) file << "\n";

				// Aktualizacja DFF'ow
				for(int y = 0; y < dff_gates.size(); y++)
				{
					dff_gates[y]->Update();
				}	

				if(ErrorFound)
				{
					//printf("Error %s Found at pos %i\n",curError->name.c_str(),x);
					ErrorCount++;
					if(test_type) break;
				}

			}

			//printf("Error %s\n"
			if(showinfo)
			{
				gotoxy(0,9);
				printf("Running test %i from %i. %i error detected [ %.2f %c ]\n",x+1,errors.size(),ErrorCount,((float)ErrorCount/errors.size())*100,'%');
			}
		}
		for(int i = 0; i < gRuns; i++)
		{
			graf[i]=graf[i]/errors.size();
		}
		if(logtofile) file.close();
	}
	void DrawGraf()
	{
		float stepx = (float)GRAFXSIZE/gRuns;
		int px = 10,py = 260;
		DrawLine(px,py,px,py-100,RGB(200,200,200));
		DrawLine(px,py,px+gRuns*stepx,py,RGB(200,200,200));
		for(int i = 0; i < gRuns; i++)
		{
			float k = graf[i];
			DrawLine(px,py,10+stepx*(i+1),260-(int)(graf[i]*100),RGB(0,255,0));
			px = 10+stepx*(i+1);
			py = 260-(int)(graf[i]*100);
		}
	}
};

int main(int argc, char* argv[])
{
	HWND hwnd = GetConsoleWindow();
	hdc = GetDC(hwnd);
	bool breaktest = false;
	bool writevectors = false;
	bool info = false;
	char *filename = new char[30];
	std::string path;
	filename[0] = '\0';
	SimBench Bench;
	GATESManager Manager;
	int vector_count;
	printf("SimBench Circuit Benchmark\nMichal Suchecki 6ISI WSTI\n\n");

	if(argc > 2)
	{
		strcpy(filename,argv[1]);
		path.append(getcwd(NULL,0));
		path.append(filename);
		printf("%s\n",path.c_str());

		vector_count = atoi(argv[2]);
		//printf("%i",vector_count);

		for(int i = 3; i < argc; i++)
		{
			if(strcmp(argv[i],"break") == 0) breaktest = true;
			if(strcmp(argv[i],"write") == 0) writevectors = true;
			if(strcmp(argv[i],"info") == 0) info = true;
		}

		if(!Bench.ParseBench(filename))
		{
			printf("Warning no %s.bench file ! Press any key to quit...\n",filename);
			return exit();
		}
		else
		{
			if(!Bench.ParseFLT(filename))
			{
				printf("Warning no %s.flt file ! Press any key to quit...\n",filename);
				return exit();
			}
		}
		Bench.Levelize();
		Manager.SetTable(maxlevel);

		printf("MAX GATE LEVEL [ %i ]\n",maxlevel);
		printf("INPUT VECTORS [ %i ]\n\n",vector_count);

		StartPerformance();
		Manager.RunBenchmark(vector_count,breaktest,writevectors,info);
		StopPerformance();

		Manager.DrawGraf();
	} 
	else 
	{
		printf("\nBrak lub zbyt mala ilosc parametrow!\n Poprawny format simbench.exe nazwa_pliku ilosc_wektorow parametry\n\nDodatkowe parametry:\n write - zapis wektorow i wyniku do pliku\n break - przerywa test po znalezieniu bledu i przechodzi do nastepnego\n\nnp simbench s27 1000 break write\n\nprogram konczy prac...");
	}
	return exit();
}