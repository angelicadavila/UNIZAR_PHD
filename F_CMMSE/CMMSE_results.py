#!/usr/bin/env python

##############################################################################
##############################################################################



import argparse
import matplotlib
import os
import numpy as np
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
w_bar=0.8
colorcycle = ['#a1dab4', '#41b6c4', '#2c7fb8', '#253494', '#4f345a', '#8fa998' ]

def main():

  os.chdir("./..")
  parser = argparse.ArgumentParser(description='Plot scheduler data.')
  parser.add_argument('fname', help='File prefix for reading the input data')
  parser.add_argument('--dir', help='Directory containing the input data.')


########################################################
########################################################
########################################################
  title_name=" "
  figure_name="CMMSE_ALL.pdf"
  fig, ax = plt.subplots(1,1,figsize=(12,4))
  #save best device
  best=0;
  

  width=0.5
  #number of devices
  n_dev=3
  #number of algorithm
  n_alg=3
  #number of benchs
  n_bars=4
  x=np.array ([1,n_dev+n_alg+3])
  print x
  st=1000000#scaling time in seconds

  bar_cycle = (cycler('hatch', ['///', '--', '...','\///', 'xxx', '\\\\']) * cycler('color', 'w')*cycler('zorder', [10]))
  styles = bar_cycle()
  

########################################################
########################################################
  os.chdir("./test_matrixmult/")
  file_test='statict_1.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=np.average(dato_cpu)/st
  std_cpu=np.std(dato_cpu)/st
  print ("CPU ",m_cpu)
  best=m_cpu
  worst=m_cpu
  
  file_test='statict_2.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=np.average(dato_gpu)/st
  std_gpu=np.std(dato_gpu)/st
  print ("GPU ",m_gpu)
  if m_gpu < best:
   best=m_gpu
  if m_gpu > worst:
   worst=m_gpu
  
  file_test='statict_4.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=np.average(dato_fpga)/st
  std_fpga=np.std(dato_fpga)/st
  print ("FPGA ",m_fpga)
  if m_fpga < best:
   best=m_fpga
  if m_fpga > worst:
   worst=m_fpga

  file_test='statict_7.txt'
  dato_st=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_st=np.average(dato_st)/st
  std_st=np.std(dato_st)/st
  if m_st > worst:
   worst=m_st
  print ("static 7 ",m_st)
  print ("speed up static 7 ",best/m_st)

  file_test='dynamict_7.txt'
  dato_dy=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_dy=np.average(dato_dy)/st
  std_dy=np.std(dato_dy)/st
  if m_dy > worst:
    worst=m_dy
  print ("dynamic 7 ",m_dy)
  print ("speedup dynamic 7 ",best/m_dy)

  file_test='hguidedt_7.txt'
  dato_hg=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_hg=np.average(dato_hg)/st
  std_hg=np.std(dato_hg)/st
  if m_dy > worst:
    worst=m_dy
  print ("hguided 7 ",m_hg)
  print ("speedup hguided 7 ",best/m_hg)


  index=0
  print best
  ax.bar(index,m_cpu/worst,yerr=std_cpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_gpu/worst,yerr=std_gpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_fpga/worst,yerr=std_fpga/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_st/worst,yerr=std_st/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_dy/worst,yerr=std_dy/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_hg/worst,yerr=std_hg/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+4

  os.chdir("./..")



########################################################
########################################################
  os.chdir("./test_mersenne")
  file_test='statict_1.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=np.average(dato_cpu)/st
  std_cpu=np.std(dato_cpu)/st
  print ("CPU ",m_cpu)
  best=m_cpu
  worst=m_cpu
  
  file_test='statict_2.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=np.average(dato_gpu)/st
  std_gpu=np.std(dato_gpu)/st
  print ("GPU ",m_gpu)
  if m_gpu < best:
   best=m_gpu
  if m_gpu > worst:
   worst=m_gpu
  
  file_test='statict_4.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=np.average(dato_fpga)/st
  std_fpga=np.std(dato_fpga)/st
  print ("FPGA ",m_fpga)
  if m_fpga < best:
   best=m_fpga
  if m_fpga > worst:
   worst=m_fpga

  file_test='statict_7.txt'
  dato_st=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_st=np.average(dato_st)/st
  std_st=np.std(dato_st)/st
  if m_st > worst:
   worst=m_st
  print ("static 7 ",m_st)
  print ("speed up static 7 ",best/m_st)

  file_test='dynamict_7.txt'
  dato_dy=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_dy=np.average(dato_dy)/st
  std_dy=np.std(dato_dy)/st
  if m_dy > worst:
    worst=m_dy
  print ("dynamic 7 ",m_dy)
  print ("speedup dynamic 7 ",best/m_dy)

  file_test='hguidedt_7.txt'
  dato_hg=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_hg=np.average(dato_hg)/st
  std_hg=np.std(dato_hg)/st
  if m_dy > worst:
    worst=m_dy
  print ("hguided 7 ",m_hg)
  print ("speedup hguided 7 ",best/m_hg)

  print best
  ax.bar(index,m_cpu/worst,yerr=std_cpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_gpu/worst,yerr=std_gpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_fpga/worst,yerr=std_fpga/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_st/worst,yerr=std_st/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_dy/worst,yerr=std_dy/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  ax.bar(index,m_hg/worst,yerr=std_hg/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+4
  os.chdir("./..")


########################################################
########################################################
  os.chdir("./test_watermark")
  file_test='statict_1.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=np.average(dato_cpu)/st
  std_cpu=np.std(dato_cpu)/st
  print ("CPU ",m_cpu)
  best=m_cpu
  worst=m_cpu
  
  file_test='statict_2.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=np.average(dato_gpu)/st
  std_gpu=np.std(dato_gpu)/st
  print ("GPU ",m_gpu)
  if m_gpu < best:
   best=m_gpu
  if m_gpu > worst:
   worst=m_gpu
  
  file_test='statict_4.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=np.average(dato_fpga)/st
  std_fpga=np.std(dato_fpga)/st
  print ("FPGA ",m_fpga)
  if m_fpga < best:
   best=m_fpga
  if m_fpga > worst:
   worst=m_fpga

  file_test='statict_7.txt'
  dato_st=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_st=np.average(dato_st)/st
  std_st=np.std(dato_st)/st
  if m_st > worst:
   worst=m_st
  print ("static 7 ",m_st)
  print ("speed up static 7 ",best/m_st)

  file_test='dynamict_7.txt'
  dato_dy=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_dy=np.average(dato_dy)/st
  std_dy=np.std(dato_dy)/st
  if m_dy > worst:
    worst=m_dy
  print ("dynamic 7 ",m_dy)
  print ("speedup dynamic 7 ",best/m_dy)

  file_test='hguidedt_7.txt'
  dato_hg=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_hg=np.average(dato_hg)/st
  std_hg=np.std(dato_hg)/st
  if m_dy > worst:
    worst=m_dy
  print ("hguided 7 ",m_hg)
  print ("speedup hguided 7 ",best/m_hg)
  
  
  print best
  rect2=ax.bar(index,m_cpu/worst,yerr=std_cpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect2=ax.bar(index,m_gpu/worst,yerr=std_gpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect2=ax.bar(index,m_fpga/worst,yerr=std_fpga/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect2=ax.bar(index,m_st/worst,yerr=std_st/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect2=ax.bar(index,m_dy/worst,yerr=std_dy/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect2=ax.bar(index,m_hg/worst,yerr=std_hg/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+4
  os.chdir("./..")


########################################################
########################################################
  os.chdir("./test_sobel")
  file_test='statict_1.txt'
  dato_cpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_cpu=np.average(dato_cpu)/st
  std_cpu=np.std(dato_cpu)/st
  print ("CPU ",m_cpu)
  best=m_cpu
  worst=m_cpu
  
  file_test='statict_2.txt'
  dato_gpu=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_gpu=np.average(dato_gpu)/st
  std_gpu=np.std(dato_gpu)/st
  print ("GPU ",m_gpu)
  if m_gpu < best:
   best=m_gpu
  if m_gpu > worst:
   worst=m_gpu
  
  file_test='statict_4.txt'
  dato_fpga=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_fpga=np.average(dato_fpga)/st
  std_fpga=np.std(dato_fpga)/st
  print ("FPGA ",m_fpga)
  if m_fpga < best:
   best=m_fpga
  if m_fpga > worst:
   worst=m_fpga

  file_test='statict_7.txt'
  dato_st=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_st=np.average(dato_st)/st
  std_st=np.std(dato_st)/st
  if m_st > worst:
   worst=m_st
  print ("static 7 ",m_st)
  print ("speed up static 7 ",best/m_st)

  file_test='dynamict_7.txt'
  dato_dy=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_dy=np.average(dato_dy)/st
  std_dy=np.std(dato_dy)/st
  if m_dy > worst:
    worst=m_dy
  print ("dynamic 7 ",m_dy)
  print ("speedup dynamic 7 ",best/m_dy)

  file_test='hguidedt_7.txt'
  dato_hg=carga_fichero(file_test,'executionKernel: ',0,1,0)   
  m_hg=np.average(dato_hg)/st
  std_hg=np.std(dato_hg)/st
  if m_dy > worst:
    worst=m_dy
  print ("hguided 7 ",m_hg)
  print ("speedup hguided 7 ",best/m_hg)

  print best
  rect3=ax.bar(index,m_cpu/worst,yerr=std_cpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect3=ax.bar(index,m_gpu/worst,yerr=std_gpu/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect3=ax.bar(index,m_fpga/worst,yerr=std_fpga/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect3=ax.bar(index,m_st/worst,yerr=std_st/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect3=ax.bar(index,m_dy/worst,yerr=std_dy/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+1
  rect3=ax.bar(index,m_hg/worst,yerr=std_hg/worst,fill=False,width=w_bar ,**next(styles)) 
  index=index+4





  text_labels= ('','                         Matrix Multiplication','','               Mersenne Twister','','   Watermarking','','Sobel Filter','','','')

  text_legend= ('CPU','GPU','FPGA','Static','Dynamic','H-guided')

# plt.xticks(index,text_labels)
  os.chdir("./..")
  index_lb=np.arange(0,28)
  print index_lb
  ax.set_xticklabels(text_labels, rotation=0, fontsize=14)
  
  minor_ticks = np.arange(0, 1, 0.02)
  ax.set_yticks(minor_ticks, minor=True)
  plt.grid(True,axis='y')
  #ax.grid(which='minor', alpha=0.4,linestyle=':')
  ax.grid(linestyle='--', linewidth=1,axis='y')
  ax.set_ylabel('Normalized Time',fontsize=16)
  ax.set_title(title_name,fontsize=14)
  plt.legend(text_legend,fontsize=14,
  loc='upper center', bbox_to_anchor=(0.5, -0.1), 
              ncol=n_alg+n_dev)
  plt.savefig(figure_name,bbox_inches='tight') 
  plt.show()




if __name__ == "__main__":
  main() 
