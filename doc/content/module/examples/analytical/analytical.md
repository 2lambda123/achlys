!config navigation breadcrumbs=True scrollspy=True

Links to other pages for this example :
- [Overview](analytical.md)
- [Set-up](module/examples/analytical/input_files.md)

# One Dimensional Analytical Solution

This example aims to demonstrate a simple set-up to get started with. 

Considering a 1-dimensional domain with a single, homogeneous trap type, a known, constant concentration, 
$c_0$, on one boundary and an initial concentration of $0$ on the other boundary. From
[!cite](PENALVA2012) following [!cite](Carslaw1959), the particle flux through second boundary as a function 
of time can be solved analytically as shown in Equation [!eqref](eq:1d_analytical) under the condition that 
$\zeta >> c_m/n_1$.

\begin{equation}
\label{eq:1d_analytical}
\varphi(t) = \frac{c_0D}{l} \left[ 1 + 2 \sum_{m=1}^{\infty}(-1)^m\exp\left(-m^2\frac{\pi^2D_{\text{eff}}t}{l^2}\right)\right]
\end{equation}

Where $D_{\text{eff}}$ is:

\begin{equation}
\label{eq:Deff}
D_{\text{eff}} = \frac{D}{1 +\zeta}
\end{equation} 

And $\zeta$ is given by:

\begin{equation}
\label{eq:zeta}
\zeta = \frac{\lambda^2n_{\text{sol}}\nu_0}{D_0n_1}\exp \left(\frac{E_{\text{diff}}- E_1}{k_BT} \right) + \frac{c_m}{n_1}
\end{equation} 

This can be simulated in Achlys and compared to the analytical solution. 
Example input parameters are shown in [table:input_parameters]
and simulation set-up will be explained in more detail below. 


Example input parameters for simple 1-dimensional effective diffusivity simulation taken from [!cite](DELAPORTE2019).

!table id=table:input_parameters caption=Example innput parameters
| parameter name  | symbol   | value | Unit |
|:-|:-|:-|:-|
| Density | $\rho$ | $3.16 \times 10^{22}$ | $m^{-3}$ |
| Lattice sites | $N_{\text{sol}}$  | $2.0 \rho$  | $m^{-3}$ |
| Trap Density           | $n_1$    | $1.00\times 10^{-1}\rho$   | $m^{-3}$ |
| Surface Concentration   | $c_0$ | $1.00\times 10^{-4} \rho$  | $m^{-3}$ |
| Diffusion rate pre-exponential factor | $D_0$ | 1.0 | $m^{2}s^{-1}$ |
| Diffusion Energy | $E_{\text{diff}}$ | 0.0 | $eV$ |
| Lattice constant | $\lambda$ | $3.16\times 10^{-8}$ | $m$ |
| De-trapping pre-exponential factor | $\nu_0$ |  $1.0\times 10^{-13}$ | $s^{-1}$ |
| De-trapping Energy, trap 1 | $E_{1}$ | $8.6\times 10^{-3}$ | $eV$ |
| Temperature |  $T_{\text{imp}}$ |  $1000$ | $K$ |
| Simulation time | $t_{\text{sim}}$ | $1.0\times 10^{-8}$ | $s$ |


The expected results are shown in Figure [fig:1d_diffusion]. 
Here the units have been converted from the raw result into SI units by multiplying by the atomic number 
density $\rho = 3.16\times 10^{22}$. Note that the differences between the simulation and 
analytical results are due to the coarse mesh used here. Accuracy can be improved by refining the 
mesh further, however, the calculation time will also increase. 

!media media/1d_diffusion.png style=width:90% id=fig:1d_diffusion caption=Comparison of Achlys with analytical case
