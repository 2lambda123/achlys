// This file is part of the MOOSE framework
// https://www.mooseframework.org
//
// All rights reserved, see COPYRIGHT for full restrictions
// https://github.com/idaholab/moose/blob/master/COPYRIGHT
//
// Licensed under LGPL 2.1, please see LICENSE for details
// https://www.gnu.org/licenses/lgpl-2.1.html

#include "TrappingMaterialConstT.h"
#include <cstdio>

registerMooseObject("achlysApp", TrappingMaterialConstT);

InputParameters TrappingMaterialConstT::validParams()
{
  InputParameters params = ADMaterial::validParams();
  params.addRequiredParam<Real>("v0", "pre-exponential detrapping factor in Arrhenious eq.");
  params.addRequiredParam<Real>("E1", "Trap detrapping energy in eV");
  params.addRequiredParam<Real>("E2", "Trap detrapping energy in eV");
  params.addRequiredParam<Real>("E3", "Trap detrapping energy in eV");
  params.addRequiredParam<Real>("k_boltz", "Boltzman constant");
  params.addRequiredParam<Real>("D0", "The diffusion pre-exponential factor");
  params.addRequiredParam<Real>("E_diff", "diffusion energy in eV");
  params.addRequiredParam<Real>("lambda", "Lattice constant in m-1");
  params.addRequiredParam<Real>("n_sol", "density of interstitial sites");
  params.addRequiredParam<Real>("n1", "possible trapping sites");
  params.addRequiredParam<Real>("n2", "possible trapping sites");
  params.addRequiredParam<Real>("n3", "possible trapping sites");
  params.addRequiredParam<Real>("const_T", "placeholder for temperature");
  return params;
}

TrappingMaterialConstT::TrappingMaterialConstT(const InputParameters & parameters)
  : ADMaterial(parameters),
    _v0(getParam<Real>("v0")),
    _E1(getParam<Real>("E1")),
    _E2(getParam<Real>("E2")),
    _E3(getParam<Real>("E3")),
    _k_boltz(getParam<Real>("k_boltz")),
    _D0(getParam<Real>("D0")),
    _E_diff(getParam<Real>("E_diff")),
    _lambda(getParam<Real>("lambda")),
    _n_sol(getParam<Real>("n_sol")),
    _const_sites_avail_type_1(getParam<Real>("n1")),
    _const_sites_avail_type_2(getParam<Real>("n2")),
    _const_sites_avail_type_3(getParam<Real>("n3")),
    _T(getParam<Real>("const_T")),
    _D(declareADProperty<Real>("D")),
    _Vm(declareADProperty<Real>("Vm")),
    _V1(declareADProperty<Real>("V1")),
    _V2(declareADProperty<Real>("V2")),
    _V3(declareADProperty<Real>("V3")),
    _n1(declareADProperty<Real>("n1")),
    _n2(declareADProperty<Real>("n2")),
    _n3(declareADProperty<Real>("n3"))
{
}

void
TrappingMaterialConstT::computeQpProperties()
{
_D[_qp] = _D0 * std::exp((-1.0 * _E_diff) / (_k_boltz * _T));
_Vm[_qp] = _D[_qp] / ( std::pow(_lambda, 2) * _n_sol);
_V1[_qp] = _v0 * std::exp((-1.0 * _E1) / (_k_boltz * _T));
_V2[_qp] = _v0 * std::exp((-1.0 * _E2) / (_k_boltz * _T));
_V3[_qp] = _v0 * std::exp((-1.0 * _E3) / (_k_boltz * _T));
_n1[_qp] = _const_sites_avail_type_1;
_n2[_qp] = _const_sites_avail_type_2;
_n3[_qp] = _const_sites_avail_type_3;
}