#! /bin/bash

files=($(echo $@ | sort))
#files=($(while (($# != 0)); do
#	find "$1" -type f -name '*.cpp'
#	find "$1" -type f -name '*.c'
#	find "$1" -type f -name '*.h'
#	shift
#done | sort | fgrep -v '/FreeRTOS/'))

echo '\documentclass[a4paper,notitlepage,10pt]{report}
\usepackage[left=2cm,right=2cm,top=2.5cm,bottom=2.5cm,footskip=1.25cm]{geometry}
\usepackage{listings,color,multicol}

\definecolor{dkgreen}{rgb}{0,0.6,0}
\definecolor{gray}{rgb}{0.5,0.5,0.5}
\definecolor{mauve}{rgb}{0.58,0,0.82}
%\newcommand{\fsize}{\small}
\newcommand{\fsize}{\tiny}
\newcommand{\tabsize}{4}

\lstdefinelanguage{diff}{
  morecomment=[f][\color{blue}]{@@},     % group identifier
  morecomment=[f][\color{red}]-,         % deleted lines
  morecomment=[f][\color{dkgreen}]+,       % added lines
  morecomment=[f][\color{magenta}]{---}, % Diff header lines (must appear after +,-)
  morecomment=[f][\color{magenta}]{+++},
}

\lstset{frame=tb,
  language=C++,
  aboveskip=0mm,
  belowskip=0mm,
  showstringspaces=false,
  columns=flexible,
  basicstyle={\fsize\ttfamily},
  numberstyle=\fsize\color{gray},
  numbers=left,
  keywordstyle=\color{blue},
  commentstyle=\color{dkgreen},
  stringstyle=\color{mauve},
  breaklines=true,
  breakatwhitespace=true,
  tabsize=\tabsize
}

\begin{document}
\columnsep=0.7cm
\begin{multicols}{2}'

for ((i = 0; i < ${#files[@]}; i++)); do
	echo "\\lstset{caption=$(echo ${files[i]} | sed 's/_/\\_/g')}"
	echo "\\lstinputlisting{${files[i]}}"
	echo
done

echo '\end{multicols}
\end{document}'
