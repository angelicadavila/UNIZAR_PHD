#!/usr/bin/env python

##############################################################################
# Adapted from: Antonio Vilches
# Revised by:   Andres Rodriguez for ViVid and Dario Suarez Gracia for CFD
# Version:      1.1
# Date:         7/32/2015
# Description:  Plotting script fro ViVid on Odroid
# Copyright:    Department Computer Architecture at University of Malaga
##############################################################################



import argparse
import matplotlib
import os
import numpy as np
import os.path as op
from itertools import izip_longest, cycle, islice
from scipy.stats.stats import pearsonr
matplotlib.use('PDF')


from cycler import cycler
#from sklearn import datasets
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.ticker import ScalarFormatter


def carga_fichero(file_name, delim, header, indice, columna):

  salida=list()
  datos=np.genfromtxt(file_name, skip_header=header,
       comments="us.")
  datos= datos[:,1]
#  new_index, new_column = 0, 1
#  for i in np.unique(datos[:,new_index]):
#    mediana=np.median(datos[datos[:,new_index] == i, new_column])
#    salida.append([i, mediana])
  return datos

##############################################################################
# Main script
#############################################################################
################################################
#Configuration variables
################################################
titlefs = 20
ylabelfs = 20
xlabelfs = 20
xticksfs = 18
yticksfs = 18
legendfs = 42
linew = 3
markers = 12
fig_width = 8
fig_height = 6
l_width=4.0
mark_s=20.0
colorcycle = ['#a1dab4', '#41b6c4', '#2c7fb8', '#253494', '#4f345a', '#8fa998' ]

monochrome = (cycler('color', ['k','grey']) * cycler('linestyle', ['-', '--', ':', '=.']) * cycler('marker', ['^', '.']))

def main():
  parser = argparse.ArgumentParser(description='Plot scheduler data.')
  parser.add_argument('fname', help='File prefix for reading the input data')
  parser.add_argument('--dir', help='Directory containing the input data.')
#  ax.set_prop_cycle(monochrome)
  
  figure_name="ACACES_FPGA_M2.pdf"
  fig, ax = plt.subplots(1,1, figsize=(5*3,3*3))
  #tex_labels= ('MM','MT','WM','SF')
  #tex_labels= ('MM','MT','WM','SF')
  tex_labels= ('Matrix Multiplication (MM)','Mersenne Twister (MT)','Watermarking (WM)','Sobel Filter (SF)')
  #filas y columnas subplot
  r=1  
  c=2
  count=0
##########################################################
############   MATRIX MULT   #############################3
##########################################################3
#
#  count=count+1
  os.chdir("./test_matrixmult/")
  
 # plt.subplot(1,1,1)

  count=count+1
  x=np.arange(1,110,10)
  p_size= (10653696*x)#/1000000000
  
  file_test='model2_sarteco4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  
  os.chdir("./..")
  i=0
  #plt.plot(x,(m_fpga/np.amax(m_fpga)),'3:',color='k',label=tex_labels[i])
  thp=np.divide(p_size,m_fpga)
  plt.plot((x[1:]*x[1:])/100,(thp[1:]/1e9),'3:',color='orange',label=tex_labels[i],linewidth=l_width, ms=mark_s )
  #plt.ylabel('Norm. Throghput',fontsize=legendfs)
  plt.xlabel('% Work Load FPGA',fontsize=legendfs)
  plt.gca().set_title(' ', fontsize=legendfs)
#  plt.gca().set_ylim(0.0)
 # plt.legend(loc='center right',fontsize=legendfs,bbox_to_anchor=(1.01,1.1))
  #plt.gca().tick_params(axis='x', labelsize=8) 
 # plt.gca().tick_params(axis='y', labelsize=8) 
  plt.grid(True)
  plt.grid(linestyle=':', linewidth=1)
  #plt.gca().set_xlim(-3)
  print("porcentaje max-min", np.amax(thp)/np.amin(thp))
##########################################################
############   MERSENNE TWISTER  #########################3
##########################################################3

 # plt.subplot(1,1,1)
  os.chdir("./test_mersenne/")
  x=np.arange(1,110,10)
  p_size= (64*4*78125*x)#/1000000000
  
  file_test='model2_sarteco4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  
  os.chdir("./..")
  i=1
  #plt.plot(x,np.divide(p_size,m_cpu[0:x.size]),'4-',color='k',label=tex_labels[i])
  #plt.plot(x,(m_fpga/np.amax(m_fpga)),'4-',color='k',label=tex_labels[i])
  thp=np.divide(p_size,m_fpga)
  plt.plot(x,(thp)/1e9,'+--',color='darksalmon',label=tex_labels[i],linewidth=l_width, ms=mark_s )
  print ("wosts_case",1-min(thp)/np.max(thp))
############################################################33
##############   Watermarking   33##########################33
############################################################33
#  count=count+1
  os.chdir("./test_watermark/")
  x=np.arange(1,110,10)
  p_size= (4*3125952*x)#/1000000000
  
  file_test='model2_sarteco4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  
  os.chdir("./..")
  i=2
  #plt.plot(x,np.divide(p_size,m_cpu[0:x.size]),'4-',color='k',label=tex_labels[i])
  #plt.plot(x,(m_fpga/np.amax(m_fpga)),'4-',color='k',label=tex_labels[i])
  thp=np.divide(p_size,m_fpga)
  plt.plot(x,(thp)/1e9,'x-',color='steelblue',label=tex_labels[i],linewidth=l_width, ms=mark_s )
  
#  
############################################################33
##############   Sobel Filter   33##########################33
############################################################33
  os.chdir("./test_sobel/")
  x=np.arange(1,110,10)
  p_size= (4*3125952*x)#/1000000000
  
  file_test='model2_sarteco4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  
  os.chdir("./..")
  i=3
  #plt.plot(x,np.divide(p_size,m_cpu[0:x.size]),'4-',color='k',label=tex_labels[i])
  #plt.plot(x,(m_fpga/np.amax(m_fpga)),'4-',color='k',label=tex_labels[i])
  thp=np.divide(p_size,m_fpga)
  plt.plot(x,(thp)/1e9,'.--',color='grey',label=tex_labels[i],linewidth=l_width, ms=mark_s)
  
  
  
  
  plt.grid(True)
  plt.grid(linestyle=':', linewidth=1)
  
  #plt.ylabel(r'Rendimiento Normalizado',fontsize=legendfs)
  plt.ylabel('Throughput ((GB/s)',fontsize=legendfs)
  plt.xlabel('% Relative Problem Size',fontsize=legendfs)
  #plt.xlabel(r'% Tama$\~n$o del problema relativo',fontsize=legendfs)
  plt.gca().set_title(' ', fontsize=legendfs)
  plt.gca().set_ylim(0.0,3.2)
  plt.gca().set_xlim(0,100)
  
  plt.gca().tick_params(axis='x', labelsize=30) 
  plt.gca().tick_params(axis='y', labelsize=30) 
  
  #plt.legend(loc='upper center', bbox_to_anchor=(-1.3, -0.3),
  #          ncol=c, fontsize=legendfs)
  plt.legend(fontsize=legendfs,bbox_to_anchor=(1.5,1.15),loc='center right',ncol=2)

  #plt.legend(loc='lower left',ncol=4)
  plt.savefig(figure_name,bbox_inches='tight') 
  plt.show()


if __name__ == "__main__":
  main() 
