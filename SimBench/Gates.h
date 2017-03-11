#pragma once

#include "Globals.h"

int maxlevel=0;

struct SGate
{
	void (SGate::*pt2Member)();		// Delegator
	// WE/WY
	int *Input[8];					// Wejscia
	int *Output;					// Wyjscie
	std::vector<SGate*> connection;	// Z kim bramka jest połączona
	// INFO
	std::string name;	// Nazwa
	int gate_level;		// Poziom bramki (ktora ma wykonac sie najpierw)
	int InputCount;		// Ilosc wejsc
	int GateType;		// Typ bramki OR,NOR,XOR,XNOR,AND,NAND,NOT,DFF
	int BitParity;		// Ilosc odczytanych 1 z pInput
	// Uszkodzenie
	int DamageIndex;	// Index wejscia
	int DamageValue;	// Wartosc bledu
	bool Damaged;		// Uszkodzone ?
	bool DamageType;	// 0 - Output, 1 - Input
public:
	SGate() : InputCount(0),GateType(-1),gate_level(-1), Damaged(false),pt2Member(NULL)
	{
		gate_level = -1;
	}
	void SetLevel(int lvl)
	{
		if(gate_level <= lvl)
		{
			if(gate_level != 0)
			{
				gate_level = lvl;

				if(gate_level > maxlevel) maxlevel = gate_level; // Maksymalny level występujacy w ukladzie

				for(int x = 0; x < connection.size(); x++)
				{
					if(connection[x]->GateType != G_DFF) connection[x]->SetLevel(gate_level+1);
				}
			}
		}
	}
	void ResetDFF()
	{
		*Output = 0;
	}
	// NEW
	inline void SetDelegator()
	{
		switch(GateType)
		{
		case G_DFF:
			pt2Member = &SGate::UpdateDFF;
			break;
		case G_NOT:
			pt2Member = &SGate::UpdateNOT;
			break;
		case G_AND:
			pt2Member = &SGate::UpdateAND;
			break;
		case G_NAND:
			pt2Member = &SGate::UpdateNAND;
			break;
		case G_OR:
			pt2Member = &SGate::UpdateOR;
			break;
		case G_NOR:
			pt2Member = &SGate::UpdateNOR;
			break;
		case G_XOR:
			pt2Member = &SGate::UpdateXOR;
			break;
		case G_XNOR:
			pt2Member = &SGate::UpdateXNOR;
			break;
		case G_INPUT:
			pt2Member = &SGate::UpdateINPUT;
			break;
		case G_OUTPUT:
			pt2Member = &SGate::UpdateOUTPUT;
			break;
		}	
	}
	inline void Update()
	{
		(this->*pt2Member)();	// Wywołanie funkcji
	}
	inline void UpdateDFF()
	{
		if(!Damaged)
		{
			*Output = *Input[0]; 
		}
		else
		{
			*Output = DamageValue;
		}
	};
	inline void UpdateNOT()
	{
		if(!Damaged)
		{
			*Output = 1;
			if(*Input[0]) *Output = 0;
		}
		else
		{
			if(DamageType)
			{
				*Output = DamageValue;
			}
			else
			{
				*Output = ( DamageValue == 1 ) ? 0 : 1; 
			}
		}
	};
	void UpdateOR() 
	{
		if(!Damaged)
		{
			// Bramka posiada minimum 2 wejscia !!
			if(*Input[0]) goto set_one;
			if(*Input[1]) goto set_one;
			
			if(InputCount > 2)
			{
				if( 3 <= InputCount && *Input[2] ) goto set_one;
				if( 4 <= InputCount && *Input[3] ) goto set_one;
			}

			if(InputCount > 4)
			{
				if( 5 <= InputCount && *Input[4]) goto set_one;
				if( 6 <= InputCount && *Input[5]) goto set_one;
				if( 7 <= InputCount && *Input[6]) goto set_one;
				if( 8 <= InputCount && *Input[7]) goto set_one;
			}
			*Output = 0;
			return;
set_one:
			*Output = 1;
		}
		else
		{
			if(!DamageType)
			{
				*Output = DamageValue;
			}
			else
			{
				BitParity = 0;
				for(int i = 0; i < InputCount; i++)
				{
					if(DamageIndex != i)
					{
						if(*Input[i]) BitParity++;
					}
					else
					{
						if(DamageValue) BitParity++;
					}
				}

				*Output = (BitParity) ? 1 : 0;
			}
		}
	}
	void UpdateNOR()
	{
		if(!Damaged)
		{
			// Bramka posiada minimum 2 wejscia !!
			if(*Input[0]) goto set_zero;
			if(*Input[1]) goto set_zero;
			
			if(InputCount > 2)
			{
				if( 3 <= InputCount && *Input[2] ) goto set_zero;
				if( 4 <= InputCount && *Input[3] ) goto set_zero;
			}

			if(InputCount > 4)
			{
				if( 5 <= InputCount && *Input[4]) goto set_zero;
				if( 6 <= InputCount && *Input[5]) goto set_zero;
				if( 7 <= InputCount && *Input[6]) goto set_zero;
				if( 8 <= InputCount && *Input[7]) goto set_zero;
			}
			*Output = 1;
			return;
set_zero:
			*Output = 0;
		}
		else
		{
			if(!DamageType)
			{
				*Output = DamageValue;
			}
			else
			{
				BitParity = 0;
				for(int i = 0; i < InputCount; i++)
				{
					if(DamageIndex != i)
					{
						if(*Input[i]) BitParity++;
					}
					else
					{
						if(DamageValue) BitParity++;
					}
				}

				*Output = (BitParity) ? 0 : 1;
			}
		}
	}
	void UpdateXOR()
	{
		BitParity = 0;
		if(!Damaged)
		{
			// 0.000253 - BitParity :/
			if(*Input[0]) BitParity++;
			if(*Input[1]) BitParity++;
			if(InputCount < 5)
			{
				if(3 <= InputCount && *Input[2]) BitParity++;
				if(4 <= InputCount && *Input[3]) BitParity++;
			}
			else
			{
				if(3 <= InputCount && *Input[2]) BitParity++;
				if(4 <= InputCount && *Input[3]) BitParity++;
				if(5 <= InputCount && *Input[4]) BitParity++;
				if(6 <= InputCount && *Input[5]) BitParity++;
				if(7 <= InputCount && *Input[6]) BitParity++;
				if(8 <= InputCount && *Input[7]) BitParity++;
			}
			*Output = ((BitParity & (1 << 0)) > 0) ? 1 : 0;
		}
		else
		{
			if(!DamageType)
			{
				*Output = DamageValue;
			}
			else
			{
				BitParity = 0;
				for(int i = 0; i < InputCount; i++)
				{
					if(DamageIndex != i)
					{
						if(*Input[i]) BitParity++;
					}
					else
					{
						if(DamageValue) BitParity++;
					}
				}

				*Output = (BitParity%2) ? 1 : 0;
			}			
		}
	}
	void UpdateXNOR() 
	{
		BitParity = 0;
		if(!Damaged)
		{
			// 0.000253 - BitParity :/
			if(*Input[0]) BitParity++;
			if(*Input[1]) BitParity++;
			if(InputCount < 5)
			{
				if(3 <= InputCount && *Input[2]) BitParity++;
				if(4 <= InputCount && *Input[3]) BitParity++;
			}
			else
			{
				if(3 <= InputCount && *Input[2]) BitParity++;
				if(4 <= InputCount && *Input[3]) BitParity++;
				if(5 <= InputCount && *Input[4]) BitParity++;
				if(6 <= InputCount && *Input[5]) BitParity++;
				if(7 <= InputCount && *Input[6]) BitParity++;
				if(8 <= InputCount && *Input[7]) BitParity++;
			}
			*Output = ((BitParity & (1 << 0)) > 0) ? 0 : 1;
		}
		else
		{
			if(!DamageType)
			{
				*Output = DamageValue;
			}
			else
			{
				BitParity = 0;
				for(int i = 0; i < InputCount; i++)
				{
					if(DamageIndex != i)
					{
						if(*Input[i]) BitParity++;
					}
					else
					{
						if(DamageValue) BitParity++;
					}
				}

				*Output = (BitParity%2) ? 0 : 1;
			}				
		}
	}
	void UpdateAND()
	{
		if(!Damaged)
		{
			// Bramka posiada minimum 2 wejscia !!
			if(!*Input[0]) goto set_zero;
			if(!*Input[1]) goto set_zero;
			
			if(InputCount > 2)
			{
				if( 3 <= InputCount && !*Input[2] ) goto set_zero;
				if( 4 <= InputCount && !*Input[3] ) goto set_zero;
			}

			if(InputCount > 4)
			{
				if( 5 <= InputCount && !*Input[4]) goto set_zero;
				if( 6 <= InputCount && !*Input[5]) goto set_zero;
				if( 7 <= InputCount && !*Input[6]) goto set_zero;
				if( 8 <= InputCount && !*Input[7]) goto set_zero;
			}
			*Output = 1;
			return;
set_zero:
			*Output = 0;
			//return;
		}
		else
		{
			if(!DamageType)
			{
				*Output = DamageValue;
			}
			else
			{
				BitParity = 0;
				for(int i = 0; i < InputCount; i++)
				{
					if(DamageIndex != i)
					{
						if(*Input[i]) BitParity++;
					}
					else
					{
						if(DamageValue) BitParity++;
					}
				}
				// Wynik
				*Output = (BitParity == InputCount) ? 1 : 0;
			}
		}
	}
	void UpdateNAND()
	{
		if(!Damaged)
		{
			// Bramka posiada minimum 2 wejscia !!
			if(!*Input[0]) goto set_one;
			if(!*Input[1]) goto set_one;
			
			if(InputCount > 2)
			{
				if( 3 <= InputCount && !*Input[2] ) goto set_one;
				if( 4 <= InputCount && !*Input[3] ) goto set_one;
			}

			if(InputCount > 4)
			{
				if( 5 <= InputCount && !*Input[4]) goto set_one;
				if( 6 <= InputCount && !*Input[5]) goto set_one;
				if( 7 <= InputCount && !*Input[6]) goto set_one;
				if( 8 <= InputCount && !*Input[7]) goto set_one;
			}
			*Output = 0;
			return;
set_one:
			*Output = 1;
			//return;
		}
		else
		{
			if(!DamageType)
			{
				*Output = DamageValue;
			}
			else
			{
				BitParity = 0;
				for(int i = 0; i < InputCount; i++)
				{
					if(DamageIndex != i)
					{
						if(*Input[i]) BitParity++;
					}
					else
					{
						if(DamageValue) BitParity++;
					}
				}
				// Wynik
				*Output = (BitParity == InputCount) ? 0 : 1;
			}
		}
	};
	void UpdateINPUT()
	{
		if(!Damaged)
		{
			*Output = *Input[0]; 
		}
		else
		{
			*Output = DamageValue;
		}
	}
	void UpdateOUTPUT()
	{
		// Nothing to do here...
	}
};