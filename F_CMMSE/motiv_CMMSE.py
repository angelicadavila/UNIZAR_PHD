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
import numpy as np
import os
import os.path as op
from itertools import izip_longest, cycle, islice

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
legendfs = 16
linew = 3
markers = 12
fig_width = 8
fig_height = 6

colorcycle = ['#a1dab4', '#41b6c4', '#2c7fb8', '#253494', '#4f345a', '#8fa998' ]

def main():
  os.chdir("./..") 
  parser = argparse.ArgumentParser(description='Plot scheduler data.')
  parser.add_argument('fname', help='File prefix for reading the input data')
  parser.add_argument('--dir', help='Directory containing the input data.')
  title_name="Watermarking "
  figure_name="CMMSE_WM.png"
  
  fig, ax = plt.subplots(1,1)
  width= 1
  x=1

  bar_cycle = (cycler('hatch', ['///', '--', '...','\///', 'xxx', '\\\\']) * cycler('color', 'w')*cycler('zorder', [10]))
  styles = bar_cycle()
  
  file_test='test_watermark/statict_1.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=np.average(dato_cpu)/1000
  print m_cpu
  std_cpu=np.std(dato_cpu)/1000

  ax.bar(x,m_cpu,yerr=std_cpu,fill=False, **next(styles)) 
  x=x+width
  file_test='test_watermark/statict_2.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=np.average(dato_gpu)/1000
  print m_gpu
  std_gpu=np.std(dato_gpu)/1000
  
  ax.bar(x,m_gpu, yerr=std_gpu,fill=False,**next(styles)) 
  x=x+width
  
  file_test='test_watermark/statict_4.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=np.average(dato_fpga)/1000
  print m_fpga
  std_fpga=np.std(dato_fpga)/1000
  
  ax.bar(x,m_fpga,yerr=std_fpga, fill=False,**next(styles)) 
  x=x+width

  file_test='test_watermark/statict_7.txt'
  dato_st=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_st=np.average(dato_st)/1000
  std_st=np.std(dato_st)/1000
  print m_st
  ax.bar(x,m_st,yerr=std_st, fill=False,**next(styles)) 
  x=x+width

  file_test='test_watermark/dynamict_7.txt'
  dato_dy=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_dy=np.average(dato_dy)/1000
  std_dy=np.std(dato_dy)/1000
  print m_dy

  ax.bar(x,m_dy,yerr=std_dy, fill=False,**next(styles)) 
  x=x+width

  file_test='test_watermark/hguidedt_7.txt'
  dato_hg=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_hg=np.average(dato_hg)/1000
  print m_hg
  std_hg=np.std(dato_hg)/1000
  
  ax.bar(x,m_hg, yerr=std_hg,fill=False,**next(styles)) 
  x=x+width


  file_test='test_watermark/statict_1.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=np.average(dato_cpu)/1000
  print m_cpu
  std_cpu=np.std(dato_cpu)/1000

  ax.bar(x,m_cpu,yerr=std_cpu,fill=False, **next(styles)) 
  x=x+width
  file_test='test_watermark/statict_2.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=np.average(dato_gpu)/1000
  print m_gpu
  std_gpu=np.std(dato_gpu)/1000
  
  ax.bar(x,m_gpu, yerr=std_gpu,fill=False,**next(styles)) 
  x=x+width
  
  file_test='test_watermark/statict_4.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=np.average(dato_fpga)/1000
  print m_fpga
  std_fpga=np.std(dato_fpga)/1000
  
  ax.bar(x,m_fpga,yerr=std_fpga, fill=False,**next(styles)) 
  x=x+width

  file_test='test_watermark/statict_7.txt'
  dato_st=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_st=np.average(dato_st)/1000
  std_st=np.std(dato_st)/1000
  print m_st
  ax.bar(x,m_st,yerr=std_st, fill=False,**next(styles)) 
  x=x+width

  file_test='test_watermark/dynamict_7.txt'
  dato_dy=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_dy=np.average(dato_dy)/1000
  std_dy=np.std(dato_dy)/1000
  print m_dy

  ax.bar(x,m_dy,yerr=std_dy, fill=False,**next(styles)) 
  x=x+width

  file_test='test_watermark/hguidedt_7.txt'
  dato_hg=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_hg=np.average(dato_hg)/1000
  print m_hg
  std_hg=np.std(dato_hg)/1000
  
  ax.bar(x,m_hg, yerr=std_hg,fill=False,**next(styles)) 
  x=x+width

  text_labels= ('CPU','CPU','GPU','FPGA','Heterogeneous')

 # plt.xticks(index,text_labels)
  ax.set_xticklabels(text_labels)
  plt.grid(True)
  ax.grid(linestyle='--', linewidth=1)
  ax.set_ylabel('Time(ms)')
  ax.set_title(title_name)
  plt.savefig(figure_name) 
  plt.show()




if __name__ == "__main__":
  main() 
