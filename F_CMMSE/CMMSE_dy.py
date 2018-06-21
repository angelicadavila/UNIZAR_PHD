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
legendfs = 16
linew = 3
markers = 12
fig_width = 8
fig_height = 6

colorcycle = ['#a1dab4', '#41b6c4', '#2c7fb8', '#253494', '#4f345a', '#8fa998' ]

monochrome = (cycler('color', ['k','grey']) * cycler('linestyle', ['-', '--', ':', '=.']) * cycler('marker', ['^', '.']))

def main():
  parser = argparse.ArgumentParser(description='Plot scheduler data.')
  parser.add_argument('fname', help='File prefix for reading the input data')
  parser.add_argument('--dir', help='Directory containing the input data.')
#  ax.set_prop_cycle(monochrome)
  
  figure_name="CMMSE_dynamic.pdf"
  fig, ax = plt.subplots(1,1, figsize=(16,2.5))
  tex_labels= ('CPU','GPU','FPGA','CPU+GPU+FPGA')
  #filas y columnas subplot
  r=1  
  c=4
  count=0
##########################################################
############   MATRIX MULT   #############################3
##########################################################3

  count=count+1
  os.chdir("./test_matrixmult/")
  x=np.arange(7,13,1)
  p_size= 1065369600/1000000000
  x=2**x 
  
  print ("Matrix Multiplication ")
  file_test='work_dynamic1t.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=dato_cpu/1000000
  print ("CPU ",m_cpu)
  file_test='statict_1.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_cpu=np.average(datos_)/1000000
  
  print ("speedup CPU",only_cpu/np.amin(m_cpu))
  
  file_test='work_dynamic2t.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=dato_gpu/1000000
  print ("gPU ",m_gpu)
  file_test='statict_2.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_gpu=np.average(datos_)/1000000
  
  print ("speedup GPU",only_gpu/np.amin(m_gpu))
  
  file_test='work_dynamic2t.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=dato_gpu/1000000
  print ("gPU ",m_gpu)
  
  file_test='work_dynamic4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  file_test='statict_4.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_fpga=np.average(datos_)/1000000
  
  print ("speedup FPGA",only_fpga/np.amin(m_fpga))
  

  sample=3
  file_test='work_dynamic7t.txt'
  datos_p=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  datos_p2=np.reshape(datos_p,(sample,8))
  dato_st=np.average(datos_p2,axis=0)
  m_st=dato_st/1000000
  print ("dynamic work 7 ",m_st)

  
  file_test='work_dynamic4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  

  sample=3
  file_test='work_dynamic7t.txt'
  datos_p=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  datos_p2=np.reshape(datos_p,(sample,8))
  dato_st=np.average(datos_p2,axis=0)
  m_st=dato_st/1000000
  print ("dynamic work 7 ",m_st)

  print ("------------ ")
  os.chdir("./..")
  print os.getcwd()
  plt.subplot(r,c,count)
  i=0
  plt.semilogx(x,p_size/m_cpu[0:x.size],'4-',color='k',label=tex_labels[i])
  i=i+1
  plt.semilogx(x,p_size/m_gpu[0:x.size],'.-',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_fpga[0:x.size],'.:',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_st[0:x.size],'h-',color='grey',label=tex_labels[i]) 
  
  plt.axhline(p_size/only_gpu, color='r')  
  plt.grid(True)
  plt.grid(linestyle='--', linewidth=1, which='major')
  ax.axis('equal')
  plt.ylabel('Throughput(GB/s)',fontsize=legendfs)
  plt.xlabel('Chunk size',fontsize=legendfs)
  plt.gca().set_title('Matrix Multiplication', fontsize=legendfs)
  plt.gca().set_ylim(0.0)
  plt.gca().set_xlim(100-2,np.amax(x)+1000)

  print("speed up bestMM",only_gpu/np.amin(m_st))
  print("speed up wortMM",only_gpu/np.amax(m_st))
##########################################################
############   MERSENNE TWISTER  #########################3
##########################################################3

  count=count+1
  os.chdir("./test_mersenne/")
  x=np.arange(7,22,1)
  p_size= 7812500*64*4/1000000000
  x=2**x 
  
  file_test='work_dynamic1t.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=dato_cpu/1000000
  print ("CPU ",m_cpu)
  file_test='statict_1.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_cpu=np.average(datos_)/1000000
  
  print ("speedup CPU",only_cpu/np.amin(m_cpu))
  
  file_test='work_dynamic2t.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=dato_gpu/1000000
  print ("gPU ",m_gpu)
  file_test='statict_2.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_gpu=np.average(datos_)/1000000
  
  print ("speedup GPU",only_gpu/np.amin(m_gpu))
  
  file_test='work_dynamic4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  file_test='statict_4.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_fpga=np.average(datos_)/1000000
  
  print ("speedup FPGA",only_fpga/np.amin(m_fpga))
  

  sample=5
  file_test='work_dynamic7t.txt'
  datos_p=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  datos_p2=np.reshape(datos_p,(sample,18))
  dato_st=np.average(datos_p2,axis=0)
  m_st=dato_st/1000000
  print ("dynamic work 7 ",m_st)

  os.chdir("./..")
  print os.getcwd()
  plt.subplot(r,c,count)
  i=0
  plt.semilogx(x,p_size/m_cpu[0:x.size],'4-',color='k',label=tex_labels[i])
  i=i+1
  plt.semilogx(x,p_size/m_gpu[0:x.size],'.-',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_fpga[0:x.size],'.:',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_st[0:x.size],'h-',color='grey',label=tex_labels[i]) 
  
  plt.axhline(p_size/only_cpu, color='r')  
  plt.grid(True)
  plt.grid(linestyle='--', linewidth=1)
#  plt.ylabel('Throughput(GB/s)',fontsize=legendfs)
  plt.xlabel('Chunk size',fontsize=legendfs)
  plt.gca().set_title('Mersenne Twister', fontsize=legendfs)
  plt.gca().set_ylim(-0.0)
###########################################################33
#############   Watermarking   33##########################33
###########################################################33
  count=count+1
  os.chdir("./test_watermark/")
  print("./test_watermark/")
  x=np.arange(10,22,1)
  p_size=312595200*4/1000000000
  x=2**x 
  print x
  
  file_test='work_dynamic1t.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=dato_cpu/1000000
  print ("CPU ",m_cpu)
  file_test='statict_1.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_cpu=np.average(datos_)/1000000
  
  print ("speedup CPU",only_cpu/np.amin(m_cpu))
  
  file_test='work_dynamic2t.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=dato_gpu/1000000
  print ("gPU ",m_gpu)
  file_test='statict_2.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_gpu=np.average(datos_)/1000000
  
  print ("speedup GPU",only_gpu/np.amin(m_gpu))
  
  file_test='work_dynamic4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  file_test='statict_4.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_fpga=np.average(datos_)/1000000
  
  print ("speedup FPGA",only_fpga/np.amin(m_fpga))

  sample=4
  file_test='work_dynamic7t.txt'
  datos_p=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  datos_p2=np.reshape(datos_p,(sample,16))
  dato_st=np.average(datos_p2,axis=0)
  m_st=dato_st/1000000
  print ("dynamic work 7 ",m_st)

  
  os.chdir("./..")
  print os.getcwd()
  plt.subplot(r,c,count)
  i=0
  plt.semilogx(x,p_size/m_cpu[0:x.size],'4-',color='k',label=tex_labels[i])
  i=i+1
  plt.semilogx(x,p_size/m_gpu[0:x.size],'.-',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_fpga[0:x.size],'.:',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_st[0:x.size],'h-',color='grey',label=tex_labels[i]) 
  
  plt.axhline(p_size/only_gpu, color='r')  
  plt.grid(True)
  plt.grid(linestyle='--', linewidth=1)
#  plt.ylabel('Throughput(GB/s)',fontsize=legendfs)
  plt.xlabel('Chunk size',fontsize=legendfs)
  plt.gca().set_title('Watermarking', fontsize=legendfs)
  plt.gca().set_ylim(-0.0)
  
###########################################################33
#############   Sobel Filter   33##########################33
###########################################################33
  
  
  count=count+1
  os.chdir("./test_sobel/")
  print("./test_sobel/")
  x=np.arange(10,28,1)
  p_size=312595200*4/1000000000
  x=2**x 
  print x
  
  file_test='work_dynamic1t.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=dato_cpu/1000000
  print ("CPU ",m_cpu)
  file_test='statict_1.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_cpu=np.average(datos_)/1000000
  
  print ("speedup CPU",only_cpu/np.amin(m_cpu))
  
  file_test='work_dynamic2t.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=dato_gpu/1000000
  print ("gPU ",m_gpu)
  file_test='statict_2.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_gpu=np.average(datos_)/1000000
  
  print ("speedup GPU",only_gpu/np.amin(m_gpu))
  
  file_test='work_dynamic4t.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=dato_fpga/1000000
  print ("FPGA ",m_fpga)
  file_test='statict_4.txt'
  datos_=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  only_fpga=np.average(datos_)/1000000
  
  print ("speedup FPGA",only_fpga/np.amin(m_fpga))

  sample=17
  file_test='work_dynamic7t.txt'
  datos_p=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  datos_p2=np.reshape(datos_p,(sample,20))
  dato_st=np.average(datos_p2,axis=0)
  m_st=dato_st/1000000
  print ("dynamic work 7 ",m_st)
  print ("Correlacion GPU-dyn sobel", pearsonr(m_st,m_gpu)) 
  
  os.chdir("./..")
  print os.getcwd()
  plt.subplot(r,c,count)
  i=0
  plt.semilogx(x,p_size/m_cpu[0:x.size],'4-',color='k',label=tex_labels[i])
  i=i+1
  plt.semilogx(x,p_size/m_gpu[0:x.size],'.-',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_fpga[0:x.size],'.:',color='k',label=tex_labels[i]) 
  i=i+1
  plt.semilogx(x,p_size/m_st[0:x.size],'h-',color='grey',label=tex_labels[i]) 
  
  plt.axhline(p_size/only_gpu, color='r')  
  plt.grid(True)
  plt.grid(linestyle='--', linewidth=1)
#  plt.ylabel('Throughput(GB/s)',fontsize=legendfs)
  plt.xlabel('Chunk size',fontsize=legendfs)
  plt.gca().set_title('Sobel Filter ', fontsize=legendfs)
  plt.gca().set_ylim(-0.0)
  
  
  plt.legend(loc='upper center', bbox_to_anchor=(-1.3, -0.3),
            ncol=c, fontsize=legendfs)
  #plt.legend(bbox_to_anchor=(1.04,1),loc='lower center')
  plt.savefig(figure_name,bbox_inches='tight') 
  plt.show()


if __name__ == "__main__":
  main() 
