#ifndef ZH_FUNCTIONS_H
#define ZH_FUNCTIONS_H

#include <cmath>
#include <vector>

#include "TLorentzVector.h"
#include "ROOT/RVec.hxx"
#include "edm4hep/ReconstructedParticleData.h"
#include "edm4hep/MCParticleData.h"
#include "edm4hep/ParticleIDData.h"

#include "ReconstructedParticle2MC.h"


ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData>  muon_quality_check(ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> in){
	
    ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> result;

    //at least one muon + and one muon - in each event
    int n_muon_plus = 0;
    int n_muon_minus = 0;
    int n = in.size();
    for(int i = 0; i < n; ++i) {
        if (in[i].charge == 1.0){
			++n_muon_plus;
        }
        else if (in[i].charge == -1.0){
            ++n_muon_minus;
        }
    }
    if(n_muon_plus >= 1 && n_muon_minus >= 1){
        result = in;
    }
    return result;
}


ROOT::VecOps::RVec<float> get_cosTheta_miss(ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> met){
    
    ROOT::VecOps::RVec<float> result;
    float costheta = 0.;
    if(met.size() > 0) {
        
        TLorentzVector lv_met;
        lv_met.SetPxPyPzE(met[0].momentum.x, met[0].momentum.y, met[0].momentum.z, met[0].energy);
        costheta = fabs(std::cos(lv_met.Theta()));

    }
    result.push_back(costheta);
    return result;
}




// Resonance builder with option to work with MC (from Emmanuel)
struct resonanceZBuilder2 {
    
    float m_resonance_mass;
    bool m_use_MC_Kinematics;
    resonanceZBuilder2(float arg_resonance_mass, bool arg_use_MC_Kinematics);
    ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> operator()(ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> legs,
                                ROOT::VecOps::RVec<int> recind,
                                ROOT::VecOps::RVec<int> mcind,
                                ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> reco,
                                ROOT::VecOps::RVec<edm4hep::MCParticleData> mc) ;
};

resonanceZBuilder2::resonanceZBuilder2(float arg_resonance_mass, bool arg_use_MC_Kinematics) {m_resonance_mass = arg_resonance_mass, m_use_MC_Kinematics = arg_use_MC_Kinematics;}

ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> resonanceZBuilder2::resonanceZBuilder2::operator()(ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> legs,
				ROOT::VecOps::RVec<int> recind ,
				ROOT::VecOps::RVec<int> mcind ,
				ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> reco ,
				ROOT::VecOps::RVec<edm4hep::MCParticleData> mc )   {

  ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> result;
  int n = legs.size();
  if (n >1) {
    ROOT::VecOps::RVec<bool> v(n);
    std::fill(v.end() - 2, v.end(), true);
    do {
      edm4hep::ReconstructedParticleData reso;
      //set initial charge == 0
      reso.charge = 0;
      TLorentzVector reso_lv; 
      for (int i = 0; i < n; ++i) {
          if (v[i]) {
            //prevent +2 and -2 charged Z 
            if (reso.charge == legs[i].charge) continue;
            reso.charge += legs[i].charge;
            TLorentzVector leg_lv;

		// Ideal detector resolution: use the kinematics of the MC particle instead
		if ( m_use_MC_Kinematics) {

		     // ugly: particles_begin is not filled in RecoParticle.
		     // hence: either need to keep trace of the index of the legs into the RecoParticle collection,
		     // or (as done below) use the track index to map the leg to the MC particle :-(

		     int track_index = legs[i].tracks_begin ;   // index in the Track array
		     int mc_index = ReconstructedParticle2MC::getTrack2MC_index( track_index, recind, mcind, reco );
		     if ( mc_index >= 0 && mc_index < (int)mc.size() ) {
			 int pdgID = mc.at( mc_index).PDG;
		         leg_lv.SetXYZM(mc.at(mc_index ).momentum.x, mc.at(mc_index ).momentum.y, mc.at(mc_index ).momentum.z, mc.at(mc_index ).mass );
		     }
		}

		else {   //use the kinematics of the reco'ed particle
		     leg_lv.SetXYZM(legs[i].momentum.x, legs[i].momentum.y, legs[i].momentum.z, legs[i].mass);
		}

            reso_lv += leg_lv;
          }
      }
      reso.momentum.x = reso_lv.Px();
      reso.momentum.y = reso_lv.Py();
      reso.momentum.z = reso_lv.Pz();
      reso.mass = reso_lv.M();
      result.emplace_back(reso);
    } while (std::next_permutation(v.begin(), v.end()));
  }
  if (result.size() > 1) {
    auto resonancesort = [&] (edm4hep::ReconstructedParticleData i ,edm4hep::ReconstructedParticleData j) { return (abs( m_resonance_mass -i.mass)<abs(m_resonance_mass-j.mass)); };
		std::sort(result.begin(), result.end(), resonancesort);
    ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData>::const_iterator first = result.begin();
    ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData>::const_iterator last = result.begin() + 1;
    ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> onlyBestReso(first, last);
    return onlyBestReso;
  } else {
    return result;
  }
}

ROOT::VecOps::RVec<float> acolinearity(ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> in){
 ROOT::VecOps::RVec<float> result;
 if(in.size() != 2) return result;

 TLorentzVector p1;
 p1.SetXYZM(in[0].momentum.x, in[0].momentum.y, in[0].momentum.z, in[0].mass);

 TLorentzVector p2;
 p2.SetXYZM(in[1].momentum.x, in[1].momentum.y, in[1].momentum.z, in[1].mass);

 float acol = abs(p1.Theta() - p2.Theta());

 result.push_back(acol);
 return result;
}



ROOT::VecOps::RVec<float> acoplanarity(ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> in){
 ROOT::VecOps::RVec<float> result;
 if(in.size() != 2) return result;

 TLorentzVector p1;
 p1.SetXYZM(in[0].momentum.x, in[0].momentum.y, in[0].momentum.z, in[0].mass);

 TLorentzVector p2;
 p2.SetXYZM(in[1].momentum.x, in[1].momentum.y, in[1].momentum.z, in[1].mass);

 float acop = abs(p1.Phi() - p2.Phi());
 if(acop > M_PI) acop = 2 * M_PI - acop;
 acop = M_PI - acop;

 result.push_back(acop);
 return result;
}


// perturb the scale of the particles
struct momentum_scale {
    momentum_scale(float arg_scaleunc);
    float scaleunc = 1.;
    ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData>  operator() (ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> in);
};

momentum_scale::momentum_scale(float arg_scaleunc) : scaleunc(arg_scaleunc) {};
ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData>  momentum_scale::operator() (ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> in) {
    ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> result;
    result.reserve(in.size());

    for (size_t i = 0; i < in.size(); ++i) {
        
        auto & p = in[i];

        /*
        TLorentzVector lv;
        lv.SetXYZM(p.momentum.x, p.momentum.y, p.momentum.z, p.mass);
        lv *= (1. + scaleunc);

        p.momentum.x = lv.Px();
        p.momentum.y = lv.Py();
        p.momentum.z = lv.Pz();
        //p.energy = lv.E();
        */



        p.momentum.x = p.momentum.x*(1. + scaleunc);
        p.momentum.y = p.momentum.y*(1. + scaleunc);
        p.momentum.z = p.momentum.z*(1. + scaleunc);
        result.emplace_back(p);
    }
    
    return result;
}


#endif
