#pragma once

#include "ADMaterial.h"
#include "LinearInterpolation.h"

class TrappingMaterialConstT;

class TrappingMaterialConstT : public ADMaterial
{
public:
  static InputParameters validParams();
  TrappingMaterialConstT(const InputParameters & parameters);

protected:
  virtual void computeQpProperties() override;

private:

  Real _v0;
  Real _E1;
  Real _E2;
  Real _E3;
  Real _k_boltz;
  Real _D0;
  Real _E_diff;
  Real _lambda;
  Real _n_sol;
  Real _const_sites_avail_type_1;
  Real _const_sites_avail_type_2;
  Real _const_sites_avail_type_3;
  Real _T;

  ADMaterialProperty<Real> & _D;
  ADMaterialProperty<Real> & _Vm;
  ADMaterialProperty<Real> & _V1;
  ADMaterialProperty<Real> & _V2;
  ADMaterialProperty<Real> & _V3;
  ADMaterialProperty<Real> & _n1;
  ADMaterialProperty<Real> & _n2;
  ADMaterialProperty<Real> & _n3;
};
