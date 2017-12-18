DOCUMENT=alg-survey
TR=techreport

FILES=*.tex 

all: $(DOCUMENT).pdf

$(DOCUMENT).pdf: $(FILES)
	#if [ -d img ]; then make -C img; fi
	rm -f pdflatex.out
	pdflatex $(DOCUMENT).tex 2>&1 | tee pdflatex.out
	if grep -q '\\cite{' *.tex; then bibtex $(DOCUMENT); fi;
	pdflatex $(DOCUMENT).tex 2>&1 | tee pdflatex.out
	pdflatex $(DOCUMENT).tex 2>&1 | tee pdflatex.out
	#dvips $(DOCUMENT).dvi -o $(DOCUMENT).ps
	#ps2pdf14 -dPDFSETTINGS=/printer -dAutoFilterColorImages=false -dColorImageFilter=/FlateEncode $(DOCUMENT).ps

tr: $(TR).pdf

$(TR).pdf: $(FILES)
	#if [ -d img ]; then make -C img; fi
	rm -f pdflatex.out
	pdflatex $(TR).tex 2>&1 | tee pdflatex.out
	if grep -q '\\cite{' *.tex; then bibtex $(TR); fi;
	pdflatex $(TR).tex 2>&1 | tee pdflatex.out
	pdflatex $(TR).tex 2>&1 | tee pdflatex.out
	#dvips $(DOCUMENT).dvi -o $(DOCUMENT).ps
	#ps2pdf14 -dPDFSETTINGS=/printer -dAutoFilterColorImages=false -dColorImageFilter=/FlateEncode $(DOCUMENT).ps
	cp $(TR).pdf ../package_for_spcl_website/sf_sc_2014/tr

show: $(DOCUMENT).pdf
	evince $(DOCUMENT).pdf

force: 
	rm -f $(DOCUMENT).pdf
	make $(DOCUMENT).pdf

clean:
	rm -f *.aux *.out $(DOCUMENT).bbl *.bst *.log
	rm -f $(DOCUMENT).blg $(DOCUMENT).dvi $(DOCUMENT).vrb
	rm -f $(DOCUMENT).pdf $(DOCUMENT).toc $(DOCUMENT).lo[a,f,g,h,l,t]
	rm -f $(DOCUMENT).nav $(DOCUMENT).snm $(DOCUMENT).ps
	rm -f history.txt llncs.cls llncs.dem llncs.ind llncsdoc.sty sprmindx.sty subjidx.ind llncs.dvi llncs.doc llncsdoc.pdf splncs03.bst readme.txt
	rm -f *.aux *.out $(TR).bbl *.bst *.log
	rm -f $(TR).blg $(TR).dvi $(TR).vrb
	rm -f $(TR).pdf $(TR).toc $(TR).lo[a,f,g,h,l,t]
	rm -f $(TR).nav $(TR).snm $(TR).ps
	#if [ -f plt/Makefile ]; then make -C plt clean; fi
	#if [ -d img ]; then make -C img clean; fi
	#if [ -d slides ]; then make -C img clean; fi
	#if [ -d ../img ]; then make -C ../img clean; fi

