#include "mezzo.h"

#include "envelope.h"

bool Envelope::allActive = true;

void Envelope::showStatus(int spaces) 
{
  using namespace std;
  cout 
    << setw(spaces) << ' ' << "Envelope: " << (allActive ? "Active" : "Inactive")
    << " [Delay:"   << delay
    << " Attack:"   << attack  << "(rate="  << attackRate << ")"
    << " Hold:"     << hold 
    << " Decay:"    << decay   << "(rate="  << decayRate  << ")"
    << " Sustain:"  << sustain 
    << " Release:"  << release << "(rate="  << releaseRate  << ")"
    << "] Att:"     << fixed   << setw(7)   << setprecision(5) << attenuation << endl;
}