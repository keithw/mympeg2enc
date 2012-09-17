source = conform.cc elemstrmwriter.cc encoderparams.cc imageplanes.cc macroblock.cc motionest.cc mpeg2coder.cc mpeg2enc.cc mpeg2encoder.cc mpeg2encoptions.cc ontheflyratectlpass1.cc ontheflyratectlpass2.cc picture.cc picturereader.cc predict.cc putpic.cc quantize.cc rate_complexity_model.cc ratectl.cc seqencoder.cc stats.cc streamstate.cc synchrolib.cc transfrm.cc cpu_accel.c mjpeg_logging.c mpegconsts.c mpegtimecode.c yuv4mpeg.c yuv4mpeg_ratio.c build_sub22_mests.c build_sub44_mests.c find_best_one_pel.c mblock_bsad_mmx.c mblock_bsumsq_mmx.c mblock_sad_mmx.c mblock_sad_mmxe.c mblock_sumsq_mmx.c motion.c motionsearch.c fdct.c fdct_mmx.c fdct_x86.c idct.c idct_mmx.c predcomp_mmx.c predcomp_mmxe.c predict_mmx.c predict_ref.c predict_x86.c quantize_ref.c quantize_x86.c quant_mmx.c tables.c transfrm_ref.c transfrm_x86.c mblock_sub44_sads_x86.c alfalfa-enc.cc
objects = conform.o elemstrmwriter.o encoderparams.o imageplanes.o macroblock.o motionest.o mpeg2coder.o mpeg2encoder.o mpeg2encoptions.o ontheflyratectlpass1.o ontheflyratectlpass2.o picture.o picturereader.o predict.o putpic.o quantize.o rate_complexity_model.o ratectl.o seqencoder.o stats.o streamstate.o synchrolib.o transfrm.o cpu_accel.o mjpeg_logging.o mpegconsts.o mpegtimecode.o yuv4mpeg.o yuv4mpeg_ratio.o build_sub22_mests.o build_sub44_mests.o find_best_one_pel.o mblock_bsad_mmx.o mblock_bsumsq_mmx.o mblock_sad_mmx.o mblock_sad_mmxe.o mblock_sumsq_mmx.o motion.o motionsearch.o fdct.o fdct_mmx.o fdct_x86.o idct.o idct_mmx.o predcomp_mmx.o predcomp_mmxe.o predict_mmx.o predict_ref.o predict_x86.o quantize_ref.o quantize_x86.o quant_mmx.o tables.o transfrm_ref.o transfrm_x86.o mblock_sub44_sads_x86.o
executables = mpeg2enc alfalfa-enc

CC = gcc
CXX = g++
CXXFLAGS = -g -Wall -pthread -I. -std=c++0x -DHAVE_CONFIG_H

all: $(executables)

mpeg2enc: mpeg2enc.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

alfalfa-enc: alfalfa-enc.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CXXFLAGS) -c -o $@ $<

-include depend

depend: $(source)
	$(CXX) $(INCLUDES) -MM $(source) > depend

.PHONY: clean
clean:
	-rm -f $(executables) depend *.o *.rpo
