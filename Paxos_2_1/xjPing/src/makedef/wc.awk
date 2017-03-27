BEGIN	{
	n1 = 0; n2 = 0; n3 = 0; s1 = 0; s2  0; s3 = 0;
	N1 = 0; N2 = 0; N3 = 0; S1 = 0; S2  0; S3 = 0; }
/^exe/	{
	printf "Total\t\t:%8d%8d%8d\t|%8d%8d%8d\n", n1, n2, n3, s1, s2, s3;
		N1 += n1; N2 += n2; N3 += n3;
		S1 += s1; S2 += s2; S3 += s3;
		n1 = 0; n2 = 0; n3 = 0; s1 = 0; s2  0; s3 = 0;
		next;
		}
/\.c/		{
		a1 = $1; a2 = $2; a3 = $3; str = $4;
		if( length($4)< 8)	str = $4 "\t";
		next;
		}
/\.h/		{
		a1 = $1; a2 = $2; a3 = $3; str = $4;
		if( length($4)< 8)	str = $4 "\t";
		next;
		}
		{
	printf "%s	:%8d%8d%8d\t|%8d%8d%8d\n",str, a1, a2, a3, $1, $2, $3;
			n1 += a1; n2 += a2; n3 += a3;
			s1 += $1; s2 += $2; s3 += $3;
		}
END		{ 
	printf "\nTotal\t\t:%8d%8d%8d\t|%8d%8d%8d\n", n1, n2, n3, s1, s2, s3;
		N1 += n1; N2 += n2; N3 += n3;
		S1 += s1; S2 += s2; S3 += s3;
	printf "All\t\t:%8d%8d%8d\t|%8d%8d%8d\n", N1, N2, N3, S1, S2, S3;
		}
