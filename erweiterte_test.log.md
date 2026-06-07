PS H:\kdna_opencl_with_inspector_krun> cd H:\kdna_opencl_with_inspector_krun
PS H:\kdna_opencl_with_inspector_krun> Get-Item .\Decode_chr17_v01\decode_hits.csv | Select-Object FullName,Length,LastWriteTime

FullName                                                            Length LastWriteTime
--------                                                            ------ -------------
H:\kdna_opencl_with_inspector_krun\Decode_chr17_v01\decode_hits.csv     31 02.06.2026 08:19:16


PS H:\kdna_opencl_with_inspector_krun> (Get-Content .\Decode_chr17_v01\decode_hits.csv -TotalCount 5)
motif,strand,pos0,pos1,window
PS H:\kdna_opencl_with_inspector_krun> (Import-Csv .\Decode_chr17_v01\decode_hits.csv | Measure-Object).Count
0
PS H:\kdna_opencl_with_inspector_krun> Remove-Item -Recurse -Force .\Decode_chr17_v01_real -ErrorAction SilentlyContinue

PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_signature_decode_v0_1.ps1 `
>>   -Root . `
>>   -Log .\Powershell.log `
>>   -Fasta .\chr17.fa `
>>   -OutDir .\Decode_chr17_v01_real
KALYX signature decoder complete: H:\kdna_opencl_with_inspector_krun\Decode_chr17_v01_real
  motifs=8 edges=17 log_windows=80 hits=86802
  report=H:\kdna_opencl_with_inspector_krun\Decode_chr17_v01_real\decode_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Item .\Decode_chr17_v01_real\decode_hits.csv | Select-Object FullName,Length,LastWriteTime

FullName                                                                  Length LastWriteTime
--------                                                                  ------ -------------
H:\kdna_opencl_with_inspector_krun\Decode_chr17_v01_real\decode_hits.csv 3211643 02.06.2026 10:26:44


PS H:\kdna_opencl_with_inspector_krun> Remove-Item -Recurse -Force .\Decode_chr17_v02_real -ErrorAction SilentlyContinue

PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_signature_cluster_v0_2.ps1 `
>>   -DecodeDir .\Decode_chr17_v01_real `
>>   -OutDir .\Decode_chr17_v02_real `
>>   -ClusterGap 32
KALYX signature cluster analyzer complete: Decode_chr17_v02_real
  hits=86802 clusters=17931 templates=79
  report=Decode_chr17_v02_real\cluster_v02_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v02_real\cluster_v02_report.md
# KALYX Signature Cluster Analyzer v0.2

## Boundary
Dieses Artefakt analysiert Motiv-Cluster, Offset-Schablonen und Cluster-Periodik. Es beweist keinen natürlichen oder künstlichen Ursprung.

## Summary
- input hits: `86802`
- clusters: `17931` using cluster_gap=32
- templates: `79`
- strand balance: plus=86611 minus=191 plus_rate=0.997799589871
- core windows 21..25: hits=86669 clusters=17835 complete_8_clusters=3219

## Top windows

| window | start0 | end0 | hits | clusters | complete_8 |
|---:|---:|---:|---:|---:|---:|
| 23 | 24117248 | 25165824 | 24222 | 4834 | 940 |
| 24 | 25165824 | 26214400 | 23962 | 4840 | 905 |
| 22 | 23068672 | 24117248 | 23072 | 4756 | 850 |
| 25 | 26214400 | 27262976 | 10953 | 2263 | 424 |
| 21 | 22020096 | 23068672 | 4460 | 1142 | 100 |
| 57 | 59768832 | 60817408 | 7 | 3 | 0 |
| 67 | 70254592 | 71303168 | 7 | 4 | 0 |
| 41 | 42991616 | 44040192 | 6 | 4 | 0 |
| 12 | 12582912 | 13631488 | 5 | 3 | 0 |
| 27 | 28311552 | 29360128 | 5 | 4 | 0 |

## Top cluster-start deltas

| delta | count |
|---:|---:|
| 170 | 3589 |
| 137 | 1874 |
| 168 | 1549 |
| 205 | 1376 |
| 506 | 1047 |
| 341 | 937 |
| 176 | 737 |
| 167 | 722 |
| 200 | 688 |
| 201 | 666 |
| 135 | 611 |
| 169 | 547 |
| 138 | 280 |
| 171 | 277 |
| 163 | 177 |
| 342 | 174 |
| 134 | 169 |
| 177 | 147 |
| 474 | 143 |
| 204 | 133 |
| 507 | 132 |
| 373 | 132 |
| 513 | 86 |
| 339 | 77 |
| 337 | 77 |

## Top templates

| rank | count | n_hits | n_motifs | strand | offsets | motifs_by_offset |
|---:|---:|---:|---:|---|---|---|
| 1 | 5397 | 6 | 6 | + | `0|2|3|4|5|6` | `ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA` |
| 2 | 3827 | 2 | 2 | + | `0|1` | `TGCATTCAACTC|GCATTCAACTCA` |
| 3 | 3174 | 8 | 8 | + | `0|2|3|4|5|6|34|35` | `ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA` |
| 4 | 1289 | 4 | 4 | + | `0|1|2|3` | `GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA` |
| 5 | 1094 | 7 | 7 | + | `0|1|2|3|4|32|33` | `AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA` |
| 6 | 935 | 1 | 1 | + | `0` | `ACAGAAGCATTC` |
| 7 | 798 | 4 | 4 | + | `0|1|29|30` | `AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA` |
| 8 | 240 | 2 | 2 | + | `0|1` | `AGCATTCTCAGA|GCATTCTCAGAA` |
| 9 | 182 | 1 | 1 | + | `0` | `GCATTCTCAGAA` |
| 10 | 165 | 1 | 1 | + | `0` | `GAAGCATTCTCA` |

## Interpretation guardrail
Der stärkste Befund ist nicht Bedeutungs-Semantik, sondern eine wiederkehrende Offset-Schablone. Das ist ein Kandidat für ein lokales Repeat-/Regulations-/Strukturmotiv und muss gegen RepeatMasker, GC-kontrollierte Shuffles und andere Chromosomen geprüft werden.
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v02_real\cluster_v02_templates.csv |
>>   Select-Object -First 20


template_rank     : 1
count             : 5397
n_hits            : 6
n_motifs          : 6
strand_set        : +
template_sha12    : 2b9f53677c2f
offsets           : 0|2|3|4|5|6
motifs_by_offset  : ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA
strands_by_offset : +|+|+|+|+|+

template_rank     : 2
count             : 3827
n_hits            : 2
n_motifs          : 2
strand_set        : +
template_sha12    : 4d7ab2a740f2
offsets           : 0|1
motifs_by_offset  : TGCATTCAACTC|GCATTCAACTCA
strands_by_offset : +|+

template_rank     : 3
count             : 3174
n_hits            : 8
n_motifs          : 8
strand_set        : +
template_sha12    : 7a803c4ad69a
offsets           : 0|2|3|4|5|6|34|35
motifs_by_offset  : ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAA
                    CTCA
strands_by_offset : +|+|+|+|+|+|+|+

template_rank     : 4
count             : 1289
n_hits            : 4
n_motifs          : 4
strand_set        : +
template_sha12    : d0a7264a18cb
offsets           : 0|1|2|3
motifs_by_offset  : GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA
strands_by_offset : +|+|+|+

template_rank     : 5
count             : 1094
n_hits            : 7
n_motifs          : 7
strand_set        : +
template_sha12    : 01b64197c168
offsets           : 0|1|2|3|4|32|33
motifs_by_offset  : AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA
strands_by_offset : +|+|+|+|+|+|+

template_rank     : 6
count             : 935
n_hits            : 1
n_motifs          : 1
strand_set        : +
template_sha12    : 37f42a5567bd
offsets           : 0
motifs_by_offset  : ACAGAAGCATTC
strands_by_offset : +

template_rank     : 7
count             : 798
n_hits            : 4
n_motifs          : 4
strand_set        : +
template_sha12    : 99b4f2bc5841
offsets           : 0|1|29|30
motifs_by_offset  : AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA
strands_by_offset : +|+|+|+

template_rank     : 8
count             : 240
n_hits            : 2
n_motifs          : 2
strand_set        : +
template_sha12    : 13ccff7fedb8
offsets           : 0|1
motifs_by_offset  : AGCATTCTCAGA|GCATTCTCAGAA
strands_by_offset : +|+

template_rank     : 9
count             : 182
n_hits            : 1
n_motifs          : 1
strand_set        : +
template_sha12    : 409373a50a55
offsets           : 0
motifs_by_offset  : GCATTCTCAGAA
strands_by_offset : +

template_rank     : 10
count             : 165
n_hits            : 1
n_motifs          : 1
strand_set        : +
template_sha12    : 8a27b477830b
offsets           : 0
motifs_by_offset  : GAAGCATTCTCA
strands_by_offset : +

template_rank     : 11
count             : 137
n_hits            : 5
n_motifs          : 5
strand_set        : +
template_sha12    : 92bf69fb8d4f
offsets           : 0|1|2|3|4
motifs_by_offset  : AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA
strands_by_offset : +|+|+|+|+

template_rank     : 12
count             : 97
n_hits            : 4
n_motifs          : 4
strand_set        : +
template_sha12    : 8face8e854b4
offsets           : 0|2|3|4
motifs_by_offset  : ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG
strands_by_offset : +|+|+|+

template_rank     : 13
count             : 92
n_hits            : 3
n_motifs          : 3
strand_set        : +
template_sha12    : 745fb28a873b
offsets           : 0|1|2
motifs_by_offset  : AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA
strands_by_offset : +|+|+

template_rank     : 14
count             : 73
n_hits            : 7
n_motifs          : 7
strand_set        : +
template_sha12    : 527c5fe17af4
offsets           : 0|2|3|4|5|34|35
motifs_by_offset  : ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|TGCATTCAACTC|GCATTCAACTCA
strands_by_offset : +|+|+|+|+|+|+

template_rank     : 15
count             : 38
n_hits            : 8
n_motifs          : 8
strand_set        : +
template_sha12    : f8b2b1fa86cc
offsets           : 0|2|3|4|5|6|33|34
motifs_by_offset  : ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAA
                    CTCA
strands_by_offset : +|+|+|+|+|+|+|+

template_rank     : 16
count             : 34
n_hits            : 5
n_motifs          : 5
strand_set        : +
template_sha12    : 124cb74ed2fd
offsets           : 0|1|2|30|31
motifs_by_offset  : AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA
strands_by_offset : +|+|+|+|+

template_rank     : 17
count             : 32
n_hits            : 2
n_motifs          : 2
strand_set        : +
template_sha12    : b43e7a6943c5
offsets           : 0|1
motifs_by_offset  : GAAGCATTCTCA|AAGCATTCTCAG
strands_by_offset : +|+

template_rank     : 18
count             : 23
n_hits            : 6
n_motifs          : 6
strand_set        : +
template_sha12    : 21e7b49e771b
offsets           : 0|1|2|3|31|32
motifs_by_offset  : GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA
strands_by_offset : +|+|+|+|+|+

template_rank     : 19
count             : 19
n_hits            : 3
n_motifs          : 3
strand_set        : +
template_sha12    : 7db4157e2ab0
offsets           : 0|2|3
motifs_by_offset  : ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA
strands_by_offset : +|+|+

template_rank     : 20
count             : 17
n_hits            : 6
n_motifs          : 6
strand_set        : +
template_sha12    : fe7bbedc9be5
offsets           : 0|2|3|4|35|36
motifs_by_offset  : ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|TGCATTCAACTC|GCATTCAACTCA
strands_by_offset : +|+|+|+|+|+



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v02_real\cluster_v02_delta_hist.csv |
>>   Select-Object -First 30

delta count
----- -----
170   3589
137   1874
168   1549
205   1376
506   1047
341   937
176   737
167   722
200   688
201   666
135   611
169   547
138   280
171   277
163   177
342   174
134   169
177   147
474   143
204   133
507   132
373   132
513   86
339   77
337   77
202   74
206   74
174   74
173   67
508   64


PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v02_real\* `
>>   -DestinationPath .\Decode_chr17_v02_real_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v02_real\cluster_v02_clusters.csv |
>>   Where-Object { $_.n_motifs -eq "8" } |
>>   Select-Object -First 20


cluster_id     : 37
start0         : 22747231
end0           : 22747278
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 59
start0         : 22751986
end0           : 22752033
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 70
start0         : 22754365
end0           : 22754412
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 79
start0         : 22756744
end0           : 22756791
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 88
start0         : 22759122
end0           : 22759169
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 99
start0         : 22761501
end0           : 22761548
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 115
start0         : 22815062
end0           : 22815109
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 119
start0         : 22816077
end0           : 22816124
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 146
start0         : 22822029
end0           : 22822076
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 156
start0         : 22824408
end0           : 22824455
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 184
start0         : 22831545
end0           : 22831592
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 193
start0         : 22833925
end0           : 22833972
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 201
start0         : 22836304
end0           : 22836351
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 211
start0         : 22838683
end0           : 22838730
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 219
start0         : 22841062
end0           : 22841109
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 229
start0         : 22843442
end0           : 22843489
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 239
start0         : 22845820
end0           : 22845867
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 248
start0         : 22848199
end0           : 22848246
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 258
start0         : 22850578
end0           : 22850625
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 267
start0         : 22852957
end0           : 22853004
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v02_real\cluster_v02_clusters.csv |
>>   Where-Object { $_.template_rank -eq "3" } |
>>   Select-Object -First 20


cluster_id     : 37
start0         : 22747231
end0           : 22747278
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 59
start0         : 22751986
end0           : 22752033
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 70
start0         : 22754365
end0           : 22754412
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 79
start0         : 22756744
end0           : 22756791
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 88
start0         : 22759122
end0           : 22759169
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 99
start0         : 22761501
end0           : 22761548
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 115
start0         : 22815062
end0           : 22815109
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 119
start0         : 22816077
end0           : 22816124
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 146
start0         : 22822029
end0           : 22822076
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 156
start0         : 22824408
end0           : 22824455
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 184
start0         : 22831545
end0           : 22831592
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 193
start0         : 22833925
end0           : 22833972
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 201
start0         : 22836304
end0           : 22836351
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 211
start0         : 22838683
end0           : 22838730
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 219
start0         : 22841062
end0           : 22841109
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 229
start0         : 22843442
end0           : 22843489
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 239
start0         : 22845820
end0           : 22845867
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 248
start0         : 22848199
end0           : 22848246
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 258
start0         : 22850578
end0           : 22850625
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C

cluster_id     : 267
start0         : 22852957
end0           : 22853004
span           : 47
n_hits         : 8
n_motifs       : 8
strand_set     : +
window         : 21
template_rank  : 3
template_sha12 : 7a803c4ad69a
motifs         : AAGCATTCTCAG|ACAGAAGCATTC|AGAAGCATTCTC|AGCATTCTCAGA|GAAGCATTCTCA|GCATTCAACTCA|GCATTCTCAGAA|TGCATTCAACT
                 C



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v02_real\cluster_v02_clusters.csv |
>>   Group-Object template_rank |
>>   Sort-Object Count -Descending |
>>   Select-Object -First 20

Count Name                      Group
----- ----                      -----
 5397 1                         {@{cluster_id=31; start0=22745873; end0=22745891; span=18; n_hits=6; n_motifs=6; str...
 3827 2                         {@{cluster_id=28; start0=22732235; end0=22732248; span=13; n_hits=2; n_motifs=2; str...
 3174 3                         {@{cluster_id=37; start0=22747231; end0=22747278; span=47; n_hits=8; n_motifs=8; str...
 1289 4                         {@{cluster_id=542; start0=22918214; end0=22918229; span=15; n_hits=4; n_motifs=4; st...
 1094 5                         {@{cluster_id=1705; start0=23200961; end0=23201006; span=45; n_hits=7; n_motifs=7; s...
  935 6                         {@{cluster_id=3; start0=3310584; end0=3310596; span=12; n_hits=1; n_motifs=1; strand...
  798 7                         {@{cluster_id=128; start0=22818461; end0=22818503; span=42; n_hits=4; n_motifs=4; st...
  240 8                         {@{cluster_id=35; start0=22746562; end0=22746575; span=13; n_hits=2; n_motifs=2; str...
  182 9                         {@{cluster_id=18; start0=11888856; end0=11888868; span=12; n_hits=1; n_motifs=1; str...
  165 10                        {@{cluster_id=2; start0=2028290; end0=2028302; span=12; n_hits=1; n_motifs=1; strand...
  137 11                        {@{cluster_id=487; start0=22904788; end0=22904804; span=16; n_hits=5; n_motifs=5; st...
   97 12                        {@{cluster_id=38; start0=22747402; end0=22747418; span=16; n_hits=4; n_motifs=4; str...
   92 13                        {@{cluster_id=221; start0=22841579; end0=22841593; span=14; n_hits=3; n_motifs=3; st...
   73 14                        {@{cluster_id=1856; start0=23234742; end0=23234789; span=47; n_hits=7; n_motifs=7; s...
   38 15                        {@{cluster_id=1935; start0=23252046; end0=23252092; span=46; n_hits=8; n_motifs=8; s...
   34 16                        {@{cluster_id=986; start0=23025452; end0=23025495; span=43; n_hits=5; n_motifs=5; st...
   32 17                        {@{cluster_id=1395; start0=23124371; end0=23124384; span=13; n_hits=2; n_motifs=2; s...
   23 18                        {@{cluster_id=726; start0=22962397; end0=22962441; span=44; n_hits=6; n_motifs=6; st...
   19 19                        {@{cluster_id=93; start0=22760143; end0=22760158; span=15; n_hits=3; n_motifs=3; str...
   17 20                        {@{cluster_id=1485; start0=23146811; end0=23146859; span=48; n_hits=6; n_motifs=6; s...


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v02_real\cluster_v02_clusters.csv |
>>   Where-Object { $_.template_rank -eq "3" } |
>>   Select-Object cluster_id,start0,end0,window,strand,n_hits,n_motifs,offsets |
>>   Select-Object -First 30


cluster_id : 37
start0     : 22747231
end0       : 22747278
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 59
start0     : 22751986
end0       : 22752033
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 70
start0     : 22754365
end0       : 22754412
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 79
start0     : 22756744
end0       : 22756791
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 88
start0     : 22759122
end0       : 22759169
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 99
start0     : 22761501
end0       : 22761548
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 115
start0     : 22815062
end0       : 22815109
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 119
start0     : 22816077
end0       : 22816124
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 146
start0     : 22822029
end0       : 22822076
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 156
start0     : 22824408
end0       : 22824455
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 184
start0     : 22831545
end0       : 22831592
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 193
start0     : 22833925
end0       : 22833972
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 201
start0     : 22836304
end0       : 22836351
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 211
start0     : 22838683
end0       : 22838730
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 219
start0     : 22841062
end0       : 22841109
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 229
start0     : 22843442
end0       : 22843489
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 239
start0     : 22845820
end0       : 22845867
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 248
start0     : 22848199
end0       : 22848246
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 258
start0     : 22850578
end0       : 22850625
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 267
start0     : 22852957
end0       : 22853004
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 277
start0     : 22855336
end0       : 22855383
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 287
start0     : 22857714
end0       : 22857761
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 297
start0     : 22860093
end0       : 22860140
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 307
start0     : 22862472
end0       : 22862519
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 317
start0     : 22864851
end0       : 22864898
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 328
start0     : 22867230
end0       : 22867277
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 338
start0     : 22869609
end0       : 22869656
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 358
start0     : 22874367
end0       : 22874414
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 367
start0     : 22876746
end0       : 22876793
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :

cluster_id : 377
start0     : 22879125
end0       : 22879172
window     : 21
strand     :
n_hits     : 8
n_motifs   : 8
offsets    :



PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_signature_block_v0_3.ps1 `
>>   -DecodeDir .\Decode_chr17_v02_real `
>>   -OutDir .\Decode_chr17_v03_real `
>>   -TargetRank 3
KALYX signature block analyzer complete: Decode_chr17_v03_real
  clusters=17931 target_rank=3 target_clusters=3174
  report=Decode_chr17_v03_real\cluster_v03_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v03_real\cluster_v03_report.md
# KALYX Signature Block Analyzer v0.3

## Boundary

Dieses Artefakt hebt v0.2-Cluster auf Blockebene: A-Modul, B-Modul, Spacer,
Cluster-Start-Deltas und Phasenraster. Es beweist keinen natürlichen oder
künstlichen Ursprung.

## Summary

- input clusters: `17931`
- templates: `79`
- target template rank: `3`
- target clusters: `3174`
- output blocks: `3174`
- top target delta: `170` count=`825`
- target delta median: `508`


## Target template

- rank: `3`
- count in v0.2: `3174`
- offsets: `0|2|3|4|5|6|34|35`
- motifs_by_offset: `ACAGAAGCATTC|AGAAGCATTCTC|GAAGCATTCTCA|AAGCATTCTCAG|AGCATTCTCAGA|GCATTCTCAGAA|TGCATTCAACTC|GCATTCAACTCA`
- span_from_offsets: `47`
- A offset range: `0`..`6`
- B offset range: `34`..`35`
- spacer_length: `16`


## Top target cluster-start deltas

| delta | count | family |
|---:|---:|---|
| 170 | 825 | 170+0 |
| 167 | 398 | 168-1 |
| 2379 | 234 | 2379+0 |
| 2033 | 220 | other |
| 169 | 161 | 168+1 |
| 2378 | 154 | 2379-1 |
| 2380 | 107 | 2379+1 |
| 2034 | 87 | other |
| 337 | 55 | other |
| 2377 | 50 | 2379-2 |
| 2376 | 48 | 2379-3 |
| 2546 | 47 | other |
| 2381 | 46 | 2379+2 |
| 338 | 46 | 341-3 |
| 508 | 45 | 508+0 |
| 2547 | 43 | other |
| 2548 | 26 | other |
| 2036 | 26 | other |
| 2035 | 24 | other |
| 2545 | 21 | other |
| 2549 | 19 | other |
| 509 | 18 | 508+1 |
| 2037 | 17 | other |
| 336 | 17 | other |
| 335 | 17 | other |

## Delta families

| family | count | rate |
|---|---:|---:|
| other | 964 | 0.303813425780 |
| 170+0 | 825 | 0.260006303183 |
| 168-1 | 398 | 0.125433343839 |
| 2379+0 | 234 | 0.073747242357 |
| 168+1 | 161 | 0.050740624015 |
| 2379-1 | 154 | 0.048534509928 |
| 2379+1 | 107 | 0.033722029625 |
| 2379-2 | 50 | 0.015757957769 |
| 2379-3 | 48 | 0.015127639458 |
| 2379+2 | 46 | 0.014497321147 |
| 341-3 | 46 | 0.014497321147 |
| 508+0 | 45 | 0.014182161992 |
| 508+1 | 18 | 0.005672864797 |
| 168+0 | 15 | 0.004727387331 |
| 508+2 | 15 | 0.004727387331 |
| 2379+3 | 12 | 0.003781909864 |
| 171+0 | 12 | 0.003781909864 |
| 507+0 | 5 | 0.001575795777 |
| 506-1 | 3 | 0.000945477466 |
| 341-2 | 3 | 0.000945477466 |
| 342+0 | 3 | 0.000945477466 |
| 506+0 | 2 | 0.000630318311 |
| 341-1 | 2 | 0.000630318311 |
| 341+0 | 2 | 0.000630318311 |
| 513+0 | 2 | 0.000630318311 |

## Phase lattice

| period | dominant_phase | count | rate |
|---:|---:|---:|---:|
| 170 | 69 | 39 | 0.012287334594 |
| 171 | 144 | 28 | 0.008821676118 |
| 337 | 243 | 21 | 0.006616257089 |
| 341 | 291 | 20 | 0.006301197227 |
| 342 | 329 | 19 | 0.005986137366 |
| 506 | 153 | 16 | 0.005040957782 |
| 507 | 119 | 14 | 0.004410838059 |
| 508 | 125 | 16 | 0.005040957782 |
| 513 | 419 | 16 | 0.005040957782 |
| 2378 | 393 | 6 | 0.001890359168 |
| 2379 | 282 | 20 | 0.006301197227 |
| 2380 | 1565 | 8 | 0.002520478891 |

## Output files

```text
cluster_v03_blocks.csv
cluster_v03_spacers.csv
cluster_v03_target_delta_hist.csv
cluster_v03_delta_families.csv
cluster_v03_phase_lattice.csv
cluster_v03_target_windows.csv
cluster_v03_manifest.json
cluster_v03_report.md
```

## Interpretation guardrail

Wenn Template rank 3 stabil span=47 und spacer=16 zeigt, ist der
stärkste aktuelle Befund ein wiederholtes A/Spacer/B-Blocksubstrat. Bedeutung,
Ursprung und biologische Funktion bleiben bis zu RepeatMasker-/GC-/Cross-
Chromosom-Kontrollen offen.
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_blocks.csv |
>>   Select-Object -First 30


cluster_id        : 37
start0            : 22747231
end0              : 22747278
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22747231
a_end0            : 22747249
b_start0          : 22747265
b_end0            : 22747278
spacer_length     : 16
prev_delta        :
next_delta        : 4755
next_delta_family : other

cluster_id        : 59
start0            : 22751986
end0              : 22752033
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22751986
a_end0            : 22752004
b_start0          : 22752020
b_end0            : 22752033
spacer_length     : 16
prev_delta        : 4755
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 70
start0            : 22754365
end0              : 22754412
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22754365
a_end0            : 22754383
b_start0          : 22754399
b_end0            : 22754412
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 79
start0            : 22756744
end0              : 22756791
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22756744
a_end0            : 22756762
b_start0          : 22756778
b_end0            : 22756791
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2378
next_delta_family : 2379-1

cluster_id        : 88
start0            : 22759122
end0              : 22759169
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22759122
a_end0            : 22759140
b_start0          : 22759156
b_end0            : 22759169
spacer_length     : 16
prev_delta        : 2378
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 99
start0            : 22761501
end0              : 22761548
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22761501
a_end0            : 22761519
b_start0          : 22761535
b_end0            : 22761548
spacer_length     : 16
prev_delta        : 2379
next_delta        : 53561
next_delta_family : other

cluster_id        : 115
start0            : 22815062
end0              : 22815109
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22815062
a_end0            : 22815080
b_start0          : 22815096
b_end0            : 22815109
spacer_length     : 16
prev_delta        : 53561
next_delta        : 1015
next_delta_family : other

cluster_id        : 119
start0            : 22816077
end0              : 22816124
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22816077
a_end0            : 22816095
b_start0          : 22816111
b_end0            : 22816124
spacer_length     : 16
prev_delta        : 1015
next_delta        : 5952
next_delta_family : other

cluster_id        : 146
start0            : 22822029
end0              : 22822076
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22822029
a_end0            : 22822047
b_start0          : 22822063
b_end0            : 22822076
spacer_length     : 16
prev_delta        : 5952
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 156
start0            : 22824408
end0              : 22824455
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22824408
a_end0            : 22824426
b_start0          : 22824442
b_end0            : 22824455
spacer_length     : 16
prev_delta        : 2379
next_delta        : 7137
next_delta_family : other

cluster_id        : 184
start0            : 22831545
end0              : 22831592
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22831545
a_end0            : 22831563
b_start0          : 22831579
b_end0            : 22831592
spacer_length     : 16
prev_delta        : 7137
next_delta        : 2380
next_delta_family : 2379+1

cluster_id        : 193
start0            : 22833925
end0              : 22833972
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22833925
a_end0            : 22833943
b_start0          : 22833959
b_end0            : 22833972
spacer_length     : 16
prev_delta        : 2380
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 201
start0            : 22836304
end0              : 22836351
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22836304
a_end0            : 22836322
b_start0          : 22836338
b_end0            : 22836351
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 211
start0            : 22838683
end0              : 22838730
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22838683
a_end0            : 22838701
b_start0          : 22838717
b_end0            : 22838730
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 219
start0            : 22841062
end0              : 22841109
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22841062
a_end0            : 22841080
b_start0          : 22841096
b_end0            : 22841109
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2380
next_delta_family : 2379+1

cluster_id        : 229
start0            : 22843442
end0              : 22843489
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22843442
a_end0            : 22843460
b_start0          : 22843476
b_end0            : 22843489
spacer_length     : 16
prev_delta        : 2380
next_delta        : 2378
next_delta_family : 2379-1

cluster_id        : 239
start0            : 22845820
end0              : 22845867
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22845820
a_end0            : 22845838
b_start0          : 22845854
b_end0            : 22845867
spacer_length     : 16
prev_delta        : 2378
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 248
start0            : 22848199
end0              : 22848246
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22848199
a_end0            : 22848217
b_start0          : 22848233
b_end0            : 22848246
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 258
start0            : 22850578
end0              : 22850625
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22850578
a_end0            : 22850596
b_start0          : 22850612
b_end0            : 22850625
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 267
start0            : 22852957
end0              : 22853004
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22852957
a_end0            : 22852975
b_start0          : 22852991
b_end0            : 22853004
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 277
start0            : 22855336
end0              : 22855383
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22855336
a_end0            : 22855354
b_start0          : 22855370
b_end0            : 22855383
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2378
next_delta_family : 2379-1

cluster_id        : 287
start0            : 22857714
end0              : 22857761
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22857714
a_end0            : 22857732
b_start0          : 22857748
b_end0            : 22857761
spacer_length     : 16
prev_delta        : 2378
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 297
start0            : 22860093
end0              : 22860140
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22860093
a_end0            : 22860111
b_start0          : 22860127
b_end0            : 22860140
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 307
start0            : 22862472
end0              : 22862519
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22862472
a_end0            : 22862490
b_start0          : 22862506
b_end0            : 22862519
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 317
start0            : 22864851
end0              : 22864898
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22864851
a_end0            : 22864869
b_start0          : 22864885
b_end0            : 22864898
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 328
start0            : 22867230
end0              : 22867277
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22867230
a_end0            : 22867248
b_start0          : 22867264
b_end0            : 22867277
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 338
start0            : 22869609
end0              : 22869656
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22869609
a_end0            : 22869627
b_start0          : 22869643
b_end0            : 22869656
spacer_length     : 16
prev_delta        : 2379
next_delta        : 4758
next_delta_family : other

cluster_id        : 358
start0            : 22874367
end0              : 22874414
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22874367
a_end0            : 22874385
b_start0          : 22874401
b_end0            : 22874414
spacer_length     : 16
prev_delta        : 4758
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 367
start0            : 22876746
end0              : 22876793
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22876746
a_end0            : 22876764
b_start0          : 22876780
b_end0            : 22876793
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0

cluster_id        : 377
start0            : 22879125
end0              : 22879172
span              : 47
window            : 21
template_rank     : 3
template_sha12    : 7a803c4ad69a
strand_set        : +
n_hits            : 8
n_motifs          : 8
a_start0          : 22879125
a_end0            : 22879143
b_start0          : 22879159
b_end0            : 22879172
spacer_length     : 16
prev_delta        : 2379
next_delta        : 2379
next_delta_family : 2379+0



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_target_delta_hist.csv |
>>   Select-Object -First 40

delta count family
----- ----- ------
170   825   170+0
167   398   168-1
2379  234   2379+0
2033  220   other
169   161   168+1
2378  154   2379-1
2380  107   2379+1
2034  87    other
337   55    other
2377  50    2379-2
2376  48    2379-3
2546  47    other
2381  46    2379+2
338   46    341-3
508   45    508+0
2547  43    other
2548  26    other
2036  26    other
2035  24    other
2545  21    other
2549  19    other
509   18    508+1
2037  17    other
336   17    other
335   17    other
168   15    168+0
510   15    508+2
1525  14    other
2382  12    2379+3
171   12    171+0
2203  12    other
2204  12    other
2716  12    other
1363  12    other
2032  10    other
1871  9     other
2551  7     other
2550  7     other
2717  7     other
2714  7     other


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_delta_families.csv |
>>   Select-Object -First 30

family count rate
------ ----- ----
other  964   0.3038134257800189
170+0  825   0.26000630318310747
168-1  398   0.12543334383863852
2379+0 234   0.07374724235739048
168+1  161   0.05074062401512764
2379-1 154   0.048534509927513396
2379+1 107   0.033722029624960605
2379-2 50    0.01575795776867318
2379-3 48    0.015127639457926252
2379+2 46    0.014497321147179325
341-3  46    0.014497321147179325
508+0  45    0.014182161991805862
508+1  18    0.005672864796722345
168+0  15    0.004727387330601954
508+2  15    0.004727387330601954
2379+3 12    0.003781909864481563
171+0  12    0.003781909864481563
507+0  5     0.001575795776867318
506-1  3     0.0009454774661203908
341-2  3     0.0009454774661203908
342+0  3     0.0009454774661203908
506+0  2     0.0006303183107469272
341-1  2     0.0006303183107469272
341+0  2     0.0006303183107469272
513+0  2     0.0006303183107469272
168-2  1     0.0003151591553734636


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_phase_lattice.csv


template_rank        : 3
period               : 170
clusters             : 3174
dominant_phase       : 69
dominant_phase_count : 39
dominant_phase_rate  : 0.012287334594

template_rank        : 3
period               : 171
clusters             : 3174
dominant_phase       : 144
dominant_phase_count : 28
dominant_phase_rate  : 0.008821676118

template_rank        : 3
period               : 337
clusters             : 3174
dominant_phase       : 243
dominant_phase_count : 21
dominant_phase_rate  : 0.006616257089

template_rank        : 3
period               : 341
clusters             : 3174
dominant_phase       : 291
dominant_phase_count : 20
dominant_phase_rate  : 0.006301197227

template_rank        : 3
period               : 342
clusters             : 3174
dominant_phase       : 329
dominant_phase_count : 19
dominant_phase_rate  : 0.005986137366

template_rank        : 3
period               : 506
clusters             : 3174
dominant_phase       : 153
dominant_phase_count : 16
dominant_phase_rate  : 0.005040957782

template_rank        : 3
period               : 507
clusters             : 3174
dominant_phase       : 119
dominant_phase_count : 14
dominant_phase_rate  : 0.004410838059

template_rank        : 3
period               : 508
clusters             : 3174
dominant_phase       : 125
dominant_phase_count : 16
dominant_phase_rate  : 0.005040957782

template_rank        : 3
period               : 513
clusters             : 3174
dominant_phase       : 419
dominant_phase_count : 16
dominant_phase_rate  : 0.005040957782

template_rank        : 3
period               : 2378
clusters             : 3174
dominant_phase       : 393
dominant_phase_count : 6
dominant_phase_rate  : 0.001890359168

template_rank        : 3
period               : 2379
clusters             : 3174
dominant_phase       : 282
dominant_phase_count : 20
dominant_phase_rate  : 0.006301197227

template_rank        : 3
period               : 2380
clusters             : 3174
dominant_phase       : 1565
dominant_phase_count : 8
dominant_phase_rate  : 0.002520478891



PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v03_real\* `
>>   -DestinationPath .\Decode_chr17_v03_real_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path `
>>     .\Decode_chr17_v01_real, `
>>     .\Decode_chr17_v02_real, `
>>     .\Decode_chr17_v03_real, `
>>     .\scripts\kalyx_signature_decode_v0_1.ps1, `
>>     .\scripts\kalyx_signature_cluster_v0_2.ps1, `
>>     .\scripts\kalyx_signature_block_v0_3.ps1, `
>>     .\python\kalyx_signature_decoder.py, `
>>     .\python\kalyx_signature_cluster_v0_2.py, `
>>     .\python\kalyx_signature_block_v0_3.py, `
>>     .\docs\KALYX_SIGNATURE_DECODER_V0_1.md, `
>>     .\docs\KALYX_SIGNATURE_CLUSTER_V0_2.md, `
>>     .\docs\KALYX_SIGNATURE_BLOCK_V0_3.md `
>>   -DestinationPath .\KALYX_signature_decode_full_package.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_spacers.csv |
>>   Group-Object spacer_length |
>>   Sort-Object Count -Descending

Count Name                      Group
----- ----                      -----
   42                           {@{template_rank=1; count=5397; n_hits=6; n_motifs=6; strand_set=+; span_from_offset...
   11 16                        {@{template_rank=3; count=3174; n_hits=8; n_motifs=8; strand_set=+; span_from_offset...
   10 17                        {@{template_rank=14; count=73; n_hits=7; n_motifs=7; strand_set=+; span_from_offsets...
    7 19                        {@{template_rank=20; count=17; n_hits=6; n_motifs=6; strand_set=+; span_from_offsets...
    3 18                        {@{template_rank=29; count=9; n_hits=6; n_motifs=6; strand_set=+; span_from_offsets=...
    3 15                        {@{template_rank=15; count=38; n_hits=8; n_motifs=8; strand_set=+; span_from_offsets...
    2 20                        {@{template_rank=21; count=17; n_hits=4; n_motifs=4; strand_set=+; span_from_offsets...
    1 13                        {@{template_rank=73; count=1; n_hits=3; n_motifs=3; strand_set=+; span_from_offsets=...


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_target_delta_hist.csv |
>>   Select-Object -First 40

delta count family
----- ----- ------
170   825   170+0
167   398   168-1
2379  234   2379+0
2033  220   other
169   161   168+1
2378  154   2379-1
2380  107   2379+1
2034  87    other
337   55    other
2377  50    2379-2
2376  48    2379-3
2546  47    other
2381  46    2379+2
338   46    341-3
508   45    508+0
2547  43    other
2548  26    other
2036  26    other
2035  24    other
2545  21    other
2549  19    other
509   18    508+1
2037  17    other
336   17    other
335   17    other
168   15    168+0
510   15    508+2
1525  14    other
2382  12    2379+3
171   12    171+0
2203  12    other
2204  12    other
2716  12    other
1363  12    other
2032  10    other
1871  9     other
2551  7     other
2550  7     other
2717  7     other
2714  7     other


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_delta_families.csv |
>>   Select-Object -First 40

family count rate
------ ----- ----
other  964   0.3038134257800189
170+0  825   0.26000630318310747
168-1  398   0.12543334383863852
2379+0 234   0.07374724235739048
168+1  161   0.05074062401512764
2379-1 154   0.048534509927513396
2379+1 107   0.033722029624960605
2379-2 50    0.01575795776867318
2379-3 48    0.015127639457926252
2379+2 46    0.014497321147179325
341-3  46    0.014497321147179325
508+0  45    0.014182161991805862
508+1  18    0.005672864796722345
168+0  15    0.004727387330601954
508+2  15    0.004727387330601954
2379+3 12    0.003781909864481563
171+0  12    0.003781909864481563
507+0  5     0.001575795776867318
506-1  3     0.0009454774661203908
341-2  3     0.0009454774661203908
342+0  3     0.0009454774661203908
506+0  2     0.0006303183107469272
341-1  2     0.0006303183107469272
341+0  2     0.0006303183107469272
513+0  2     0.0006303183107469272
168-2  1     0.0003151591553734636


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v03_real\cluster_v03_target_windows.csv |
>>   Sort-Object {[int]$_.blocks} -Descending |
>>   Select-Object -First 10

window target_clusters start_min start_max
------ --------------- --------- ---------
25     418             26215280  26712566
21     100             22747231  23068274
22     836             23070653  24115037
23     925             24117415  25165586
24     895             25167622  26212902


PS H:\kdna_opencl_with_inspector_krun>

PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_signature_sequence_v0_4.ps1 `
>>   -BlockDir .\Decode_chr17_v03_real `
>>   -Fasta .\chr17.fa `
>>   -OutDir .\Decode_chr17_v04_real `
>>   -TargetRank 3
KALYX signature sequence decoder complete: Decode_chr17_v04_real
  blocks=3174 target_rank=3 target_blocks=3174 ok=3174
  unique_spacers=92 unique_full47=92
  report=Decode_chr17_v04_real\cluster_v04_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v04_real\cluster_v04_report.md
# KALYX Signature Block Sequence Decoder v0.4

## Boundary

Dieses Artefakt extrahiert die reale 47-bp-Sequenz aus `chr17.fa` für die v0.3-Blöcke.
Es analysiert A-Modul, 16-bp-Spacer, B-Modul, Varianten und Delta-Kopplungen.
Es beweist keinen natürlichen oder künstlichen Ursprung.

## Summary

- input v0.3 blocks: `3174`
- target template rank: `3`
- target blocks: `3174`
- validation ok: `3174`
- unique full_47bp variants: `92`
- unique spacer_16bp variants: `92`
- top spacer: `CCTTCTTCGTGATGTC` count=`1140`
- top full_47bp: `ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTCTGCATTCAACTCA` count=`1140`
- median spacer GC: `0.500000000000`

## Expected module validation

- expected A: `ACAGAAGCATTCTCAGAA`
- expected B: `TGCATTCAACTCA`

## Top spacer variants

| rank | spacer | count | rate | GC | entropy |
|---:|---|---:|---:|---:|---:|
| 1 | `CCTTCTTCGTGATGTC` | 1140 | 0.359168241966 | 0.500000000000 | 1.748999223062 |
| 2 | `CCCTCTTCGTGATGTT` | 1027 | 0.323566477631 | 0.500000000000 | 1.748999223062 |
| 3 | `CCTTCTTCGTGATGTT` | 588 | 0.185255198488 | 0.437500000000 | 1.702819531115 |
| 4 | `ACTTTTCTGTGATGAC` | 169 | 0.053245116572 | 0.375000000000 | 1.880240814944 |
| 5 | `GCTTCTCTGTGATGAC` | 52 | 0.016383112791 | 0.500000000000 | 1.905639062230 |
| 6 | `ACTCCTTTGTGATGTT` | 13 | 0.004095778198 | 0.375000000000 | 1.780639062230 |
| 7 | `ACTTCTTTGTGATGTG` | 10 | 0.003150598614 | 0.375000000000 | 1.750000000000 |
| 8 | `CCTTCTTCATGATGTC` | 7 | 0.002205419030 | 0.437500000000 | 1.796179691947 |
| 9 | `CCTTCTTCGTGGTGTC` | 7 | 0.002205419030 | 0.562500000000 | 1.546179691947 |
| 10 | `CCTTCTTCGTGAGGTT` | 7 | 0.002205419030 | 0.500000000000 | 1.771782221600 |
| 11 | `CCCTCTTCGTGATGTC` | 6 | 0.001890359168 | 0.562500000000 | 1.764097655574 |
| 12 | `CCTTCTTCGTGATGTG` | 6 | 0.001890359168 | 0.500000000000 | 1.771782221600 |
| 13 | `CCTCCTTCGTGATGTC` | 5 | 0.001575299307 | 0.562500000000 | 1.764097655574 |
| 14 | `CACTCTTCGTGATGTT` | 5 | 0.001575299307 | 0.437500000000 | 1.849601752715 |
| 15 | `ACTTCTTTGTGATGTT` | 5 | 0.001575299307 | 0.312500000000 | 1.669736717803 |
| 16 | `CTTTCTTCGTGATGTT` | 4 | 0.001260239445 | 0.375000000000 | 1.622556248918 |
| 17 | `TCCTCTTCGTGATGTT` | 4 | 0.001260239445 | 0.437500000000 | 1.702819531115 |
| 18 | `CCTTCTTCGTGCTGTC` | 4 | 0.001260239445 | 0.562500000000 | 1.505240814944 |
| 19 | `TCTTCTTCGTGATGAT` | 4 | 0.001260239445 | 0.375000000000 | 1.780639062230 |
| 20 | `CCCTCTTCCTGATGTT` | 4 | 0.001260239445 | 0.500000000000 | 1.677421283829 |
| 21 | `TCTTCTTCGTGATGTC` | 4 | 0.001260239445 | 0.437500000000 | 1.702819531115 |
| 22 | `ACTTCTTTGTGATGCT` | 3 | 0.000945179584 | 0.375000000000 | 1.780639062230 |
| 23 | `CGTTCTTCGTGATGTC` | 3 | 0.000945179584 | 0.500000000000 | 1.771782221600 |
| 24 | `CCTTCTTCCTGATGTC` | 3 | 0.000945179584 | 0.500000000000 | 1.677421283829 |
| 25 | `CCCTCTTGGTGATGTT` | 3 | 0.000945179584 | 0.500000000000 | 1.771782221600 |
| 26 | `CATTCTTCGTGATGTT` | 3 | 0.000945179584 | 0.375000000000 | 1.780639062230 |
| 27 | `CCCTCTTCGTGACGTT` | 3 | 0.000945179584 | 0.562500000000 | 1.764097655574 |
| 28 | `CCTTCTTCTTGATGTC` | 3 | 0.000945179584 | 0.437500000000 | 1.649397470348 |
| 29 | `CCTACTTCGTGATGTT` | 3 | 0.000945179584 | 0.437500000000 | 1.849601752715 |
| 30 | `ACTACTTTGTGATGTG` | 3 | 0.000945179584 | 0.375000000000 | 1.849601752715 |
| 31 | `CCCTCTTCGAGATGTT` | 2 | 0.000630119723 | 0.500000000000 | 1.882856063692 |
| 32 | `CCCTCGTCGTGATGTT` | 2 | 0.000630119723 | 0.562500000000 | 1.805036532577 |
| 33 | `CCCTCTTCGTGATGCT` | 2 | 0.000630119723 | 0.562500000000 | 1.764097655574 |
| 34 | `CCTTCTTCGAGATGTC` | 2 | 0.000630119723 | 0.500000000000 | 1.882856063692 |
| 35 | `TCTTCTTCGTGATGTT` | 2 | 0.000630119723 | 0.375000000000 | 1.622556248918 |
| 36 | `CCTTCCTCGTGATGTC` | 2 | 0.000630119723 | 0.562500000000 | 1.764097655574 |
| 37 | `CCTTCTTCATGATGTT` | 2 | 0.000630119723 | 0.375000000000 | 1.750000000000 |
| 38 | `CCCTCTTCGTGGTGTT` | 2 | 0.000630119723 | 0.562500000000 | 1.546179691947 |
| 39 | `CCTTCTTCGTGATATC` | 2 | 0.000630119723 | 0.437500000000 | 1.796179691947 |
| 40 | `CCATCTTCGTGATGTC` | 2 | 0.000630119723 | 0.500000000000 | 1.882856063692 |

## Top full 47-bp variants

| rank | full_47bp | count | rate |
|---:|---|---:|---:|
| 1 | `ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTCTGCATTCAACTCA` | 1140 | 0.359168241966 |
| 2 | `ACAGAAGCATTCTCAGAACCCTCTTCGTGATGTTTGCATTCAACTCA` | 1027 | 0.323566477631 |
| 3 | `ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTTTGCATTCAACTCA` | 588 | 0.185255198488 |
| 4 | `ACAGAAGCATTCTCAGAAACTTTTCTGTGATGACTGCATTCAACTCA` | 169 | 0.053245116572 |
| 5 | `ACAGAAGCATTCTCAGAAGCTTCTCTGTGATGACTGCATTCAACTCA` | 52 | 0.016383112791 |
| 6 | `ACAGAAGCATTCTCAGAAACTCCTTTGTGATGTTTGCATTCAACTCA` | 13 | 0.004095778198 |
| 7 | `ACAGAAGCATTCTCAGAAACTTCTTTGTGATGTGTGCATTCAACTCA` | 10 | 0.003150598614 |
| 8 | `ACAGAAGCATTCTCAGAACCTTCTTCATGATGTCTGCATTCAACTCA` | 7 | 0.002205419030 |
| 9 | `ACAGAAGCATTCTCAGAACCTTCTTCGTGGTGTCTGCATTCAACTCA` | 7 | 0.002205419030 |
| 10 | `ACAGAAGCATTCTCAGAACCTTCTTCGTGAGGTTTGCATTCAACTCA` | 7 | 0.002205419030 |
| 11 | `ACAGAAGCATTCTCAGAACCCTCTTCGTGATGTCTGCATTCAACTCA` | 6 | 0.001890359168 |
| 12 | `ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTGTGCATTCAACTCA` | 6 | 0.001890359168 |
| 13 | `ACAGAAGCATTCTCAGAACCTCCTTCGTGATGTCTGCATTCAACTCA` | 5 | 0.001575299307 |
| 14 | `ACAGAAGCATTCTCAGAACACTCTTCGTGATGTTTGCATTCAACTCA` | 5 | 0.001575299307 |
| 15 | `ACAGAAGCATTCTCAGAAACTTCTTTGTGATGTTTGCATTCAACTCA` | 5 | 0.001575299307 |
| 16 | `ACAGAAGCATTCTCAGAACTTTCTTCGTGATGTTTGCATTCAACTCA` | 4 | 0.001260239445 |
| 17 | `ACAGAAGCATTCTCAGAATCCTCTTCGTGATGTTTGCATTCAACTCA` | 4 | 0.001260239445 |
| 18 | `ACAGAAGCATTCTCAGAACCTTCTTCGTGCTGTCTGCATTCAACTCA` | 4 | 0.001260239445 |
| 19 | `ACAGAAGCATTCTCAGAATCTTCTTCGTGATGATTGCATTCAACTCA` | 4 | 0.001260239445 |
| 20 | `ACAGAAGCATTCTCAGAACCCTCTTCCTGATGTTTGCATTCAACTCA` | 4 | 0.001260239445 |
| 21 | `ACAGAAGCATTCTCAGAATCTTCTTCGTGATGTCTGCATTCAACTCA` | 4 | 0.001260239445 |
| 22 | `ACAGAAGCATTCTCAGAAACTTCTTTGTGATGCTTGCATTCAACTCA` | 3 | 0.000945179584 |
| 23 | `ACAGAAGCATTCTCAGAACGTTCTTCGTGATGTCTGCATTCAACTCA` | 3 | 0.000945179584 |
| 24 | `ACAGAAGCATTCTCAGAACCTTCTTCCTGATGTCTGCATTCAACTCA` | 3 | 0.000945179584 |
| 25 | `ACAGAAGCATTCTCAGAACCCTCTTGGTGATGTTTGCATTCAACTCA` | 3 | 0.000945179584 |

## Top delta-family x spacer states

| rank | next_delta_family | spacer | count | rate |
|---:|---|---|---:|---:|
| 1 | `170+0` | `CCTTCTTCGTGATGTC` | 780 | 0.245746691871 |
| 2 | `other` | `CCCTCTTCGTGATGTT` | 602 | 0.189666036547 |
| 3 | `168-1` | `CCCTCTTCGTGATGTT` | 363 | 0.114366729679 |
| 4 | `168+1` | `CCTTCTTCGTGATGTC` | 155 | 0.048834278513 |
| 5 | `2379+0` | `CCTTCTTCGTGATGTT` | 138 | 0.043478260870 |
| 6 | `other` | `CCTTCTTCGTGATGTC` | 134 | 0.042218021424 |
| 7 | `2379-1` | `CCTTCTTCGTGATGTT` | 127 | 0.040012602394 |
| 8 | `other` | `CCTTCTTCGTGATGTT` | 112 | 0.035286704474 |
| 9 | `2379+0` | `ACTTTTCTGTGATGAC` | 74 | 0.023314429742 |
| 10 | `2380+0` | `CCTTCTTCGTGATGTT` | 64 | 0.020163831128 |
| 11 | `2379-2` | `CCTTCTTCGTGATGTT` | 45 | 0.014177693762 |
| 12 | `508+0` | `GCTTCTCTGTGATGAC` | 44 | 0.013862633900 |
| 13 | `341-3` | `CCTTCTTCGTGATGTC` | 44 | 0.013862633900 |
| 14 | `2379-3` | `CCTTCTTCGTGATGTT` | 43 | 0.013547574039 |
| 15 | `other` | `ACTTTTCTGTGATGAC` | 41 | 0.012917454316 |
| 16 | `2380+1` | `CCTTCTTCGTGATGTT` | 29 | 0.009136735980 |
| 17 | `2380+0` | `ACTTTTCTGTGATGAC` | 24 | 0.007561436673 |
| 18 | `2379+0` | `CCCTCTTCGTGATGTT` | 18 | 0.005671077505 |
| 19 | `2379-1` | `CCCTCTTCGTGATGTT` | 13 | 0.004095778198 |
| 20 | `508+1` | `CCTTCTTCGTGATGTT` | 13 | 0.004095778198 |
| 21 | `2380+1` | `ACTTTTCTGTGATGAC` | 11 | 0.003465658475 |
| 22 | `508+2` | `ACTCCTTTGTGATGTT` | 11 | 0.003465658475 |
| 23 | `2380+0` | `CCCTCTTCGTGATGTT` | 10 | 0.003150598614 |
| 24 | `2379-1` | `ACTTTTCTGTGATGAC` | 9 | 0.002835538752 |
| 25 | `171+0` | `CCTTCTTCGTGATGTC` | 9 | 0.002835538752 |
| 26 | `168+0` | `CCCTCTTCGTGATGTT` | 8 | 0.002520478891 |
| 27 | `170+0` | `CCTTCTTCGTGGTGTC` | 7 | 0.002205419030 |
| 28 | `other` | `ACTTCTTTGTGATGTG` | 7 | 0.002205419030 |
| 29 | `168-1` | `CCTTCTTCGTGATGTT` | 6 | 0.001890359168 |
| 30 | `2380+2` | `ACTTTTCTGTGATGAC` | 5 | 0.001575299307 |
| 31 | `2380+2` | `CCTTCTTCGTGATGTT` | 5 | 0.001575299307 |
| 32 | `other` | `GCTTCTCTGTGATGAC` | 5 | 0.001575299307 |
| 33 | `170+0` | `CCTCCTTCGTGATGTC` | 5 | 0.001575299307 |
| 34 | `168+0` | `CCTTCTTCGTGATGTT` | 5 | 0.001575299307 |
| 35 | `168+1` | `CCCTCTTCGTGATGTT` | 4 | 0.001260239445 |
| 36 | `168-1` | `CACTCTTCGTGATGTT` | 4 | 0.001260239445 |
| 37 | `2380+0` | `CCTTCTTCGTGATGTC` | 4 | 0.001260239445 |
| 38 | `507+0` | `CCTTCTTCGTGAGGTT` | 4 | 0.001260239445 |
| 39 | `170+0` | `CGTTCTTCGTGATGTC` | 3 | 0.000945179584 |
| 40 | `other` | `CTTTCTTCGTGATGTT` | 3 | 0.000945179584 |

## Output files

```text
cluster_v04_block_sequences.csv
cluster_v04_spacer_variants.csv
cluster_v04_full47_variants.csv
cluster_v04_a_variants.csv
cluster_v04_b_variants.csv
cluster_v04_delta_spacer.csv
cluster_v04_window_spacer.csv
cluster_v04_validation.csv
cluster_v04_manifest.json
cluster_v04_report.md
```

## Interpretation guardrail

Wenn der Spacer wenige dominante Zustände besitzt, ist er Teil der Syntax.
Wenn der Spacer sehr variabel ist, ist er eher ein Abstandsfeld/Strukturfenster.
Beides bleibt erst nach RepeatMasker-, GC-Shuffle- und Cross-Chromosom-Kontrollen biologisch interpretierbar.
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v04_real\cluster_v04_spacer_variants.csv |
>>   Select-Object -First 40


rank           : 1
spacer_16bp    : CCTTCTTCGTGATGTC
count          : 1140
rate           : 0.359168241966
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.748999223062
low_complexity : 0

rank           : 2
spacer_16bp    : CCCTCTTCGTGATGTT
count          : 1027
rate           : 0.323566477631
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.748999223062
low_complexity : 0

rank           : 3
spacer_16bp    : CCTTCTTCGTGATGTT
count          : 588
rate           : 0.185255198488
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.702819531115
low_complexity : 0

rank           : 4
spacer_16bp    : ACTTTTCTGTGATGAC
count          : 169
rate           : 0.053245116572
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.880240814944
low_complexity : 0

rank           : 5
spacer_16bp    : GCTTCTCTGTGATGAC
count          : 52
rate           : 0.016383112791
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.905639062230
low_complexity : 0

rank           : 6
spacer_16bp    : ACTCCTTTGTGATGTT
count          : 13
rate           : 0.004095778198
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.780639062230
low_complexity : 0

rank           : 7
spacer_16bp    : ACTTCTTTGTGATGTG
count          : 10
rate           : 0.003150598614
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.750000000000
low_complexity : 0

rank           : 8
spacer_16bp    : CCTTCTTCATGATGTC
count          : 7
rate           : 0.002205419030
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.796179691947
low_complexity : 0

rank           : 9
spacer_16bp    : CCTTCTTCGTGGTGTC
count          : 7
rate           : 0.002205419030
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.546179691947
low_complexity : 0

rank           : 10
spacer_16bp    : CCTTCTTCGTGAGGTT
count          : 7
rate           : 0.002205419030
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.771782221600
low_complexity : 0

rank           : 11
spacer_16bp    : CCCTCTTCGTGATGTC
count          : 6
rate           : 0.001890359168
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.764097655574
low_complexity : 0

rank           : 12
spacer_16bp    : CCTTCTTCGTGATGTG
count          : 6
rate           : 0.001890359168
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.771782221600
low_complexity : 0

rank           : 13
spacer_16bp    : CCTCCTTCGTGATGTC
count          : 5
rate           : 0.001575299307
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.764097655574
low_complexity : 0

rank           : 14
spacer_16bp    : CACTCTTCGTGATGTT
count          : 5
rate           : 0.001575299307
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.849601752715
low_complexity : 0

rank           : 15
spacer_16bp    : ACTTCTTTGTGATGTT
count          : 5
rate           : 0.001575299307
length         : 16
gc             : 0.312500000000
at             : 0.687500000000
entropy_bits   : 1.669736717803
low_complexity : 0

rank           : 16
spacer_16bp    : CTTTCTTCGTGATGTT
count          : 4
rate           : 0.001260239445
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.622556248918
low_complexity : 0

rank           : 17
spacer_16bp    : TCCTCTTCGTGATGTT
count          : 4
rate           : 0.001260239445
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.702819531115
low_complexity : 0

rank           : 18
spacer_16bp    : CCTTCTTCGTGCTGTC
count          : 4
rate           : 0.001260239445
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.505240814944
low_complexity : 0

rank           : 19
spacer_16bp    : TCTTCTTCGTGATGAT
count          : 4
rate           : 0.001260239445
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.780639062230
low_complexity : 0

rank           : 20
spacer_16bp    : CCCTCTTCCTGATGTT
count          : 4
rate           : 0.001260239445
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.677421283829
low_complexity : 0

rank           : 21
spacer_16bp    : TCTTCTTCGTGATGTC
count          : 4
rate           : 0.001260239445
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.702819531115
low_complexity : 0

rank           : 22
spacer_16bp    : ACTTCTTTGTGATGCT
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.780639062230
low_complexity : 0

rank           : 23
spacer_16bp    : CGTTCTTCGTGATGTC
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.771782221600
low_complexity : 0

rank           : 24
spacer_16bp    : CCTTCTTCCTGATGTC
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.677421283829
low_complexity : 0

rank           : 25
spacer_16bp    : CCCTCTTGGTGATGTT
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.771782221600
low_complexity : 0

rank           : 26
spacer_16bp    : CATTCTTCGTGATGTT
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.780639062230
low_complexity : 0

rank           : 27
spacer_16bp    : CCCTCTTCGTGACGTT
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.764097655574
low_complexity : 0

rank           : 28
spacer_16bp    : CCTTCTTCTTGATGTC
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.649397470348
low_complexity : 0

rank           : 29
spacer_16bp    : CCTACTTCGTGATGTT
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.849601752715
low_complexity : 0

rank           : 30
spacer_16bp    : ACTACTTTGTGATGTG
count          : 3
rate           : 0.000945179584
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.849601752715
low_complexity : 0

rank           : 31
spacer_16bp    : CCCTCTTCGAGATGTT
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.882856063692
low_complexity : 0

rank           : 32
spacer_16bp    : CCCTCGTCGTGATGTT
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.805036532577
low_complexity : 0

rank           : 33
spacer_16bp    : CCCTCTTCGTGATGCT
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.764097655574
low_complexity : 0

rank           : 34
spacer_16bp    : CCTTCTTCGAGATGTC
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.882856063692
low_complexity : 0

rank           : 35
spacer_16bp    : TCTTCTTCGTGATGTT
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.622556248918
low_complexity : 0

rank           : 36
spacer_16bp    : CCTTCCTCGTGATGTC
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.764097655574
low_complexity : 0

rank           : 37
spacer_16bp    : CCTTCTTCATGATGTT
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.375000000000
at             : 0.625000000000
entropy_bits   : 1.750000000000
low_complexity : 0

rank           : 38
spacer_16bp    : CCCTCTTCGTGGTGTT
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.562500000000
at             : 0.437500000000
entropy_bits   : 1.546179691947
low_complexity : 0

rank           : 39
spacer_16bp    : CCTTCTTCGTGATATC
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.437500000000
at             : 0.562500000000
entropy_bits   : 1.796179691947
low_complexity : 0

rank           : 40
spacer_16bp    : CCATCTTCGTGATGTC
count          : 2
rate           : 0.000630119723
length         : 16
gc             : 0.500000000000
at             : 0.500000000000
entropy_bits   : 1.882856063692
low_complexity : 0



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v04_real\cluster_v04_full47_variants.csv |
>>   Select-Object -First 30


rank           : 1
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTCTGCATTCAACTCA
count          : 1140
rate           : 0.359168241966
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.955314680723
low_complexity : 0

rank           : 2
full_47bp      : ACAGAAGCATTCTCAGAACCCTCTTCGTGATGTTTGCATTCAACTCA
count          : 1027
rate           : 0.323566477631
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.955314680723
low_complexity : 0

rank           : 3
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTTTGCATTCAACTCA
count          : 588
rate           : 0.185255198488
length         : 47
gc             : 0.404255319149
at             : 0.595744680851
entropy_bits   : 1.950756729699
low_complexity : 0

rank           : 4
full_47bp      : ACAGAAGCATTCTCAGAAACTTTTCTGTGATGACTGCATTCAACTCA
count          : 169
rate           : 0.053245116572
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.945832253755
low_complexity : 0

rank           : 5
full_47bp      : ACAGAAGCATTCTCAGAAGCTTCTCTGTGATGACTGCATTCAACTCA
count          : 52
rate           : 0.016383112791
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.971009378203
low_complexity : 0

rank           : 6
full_47bp      : ACAGAAGCATTCTCAGAAACTCCTTTGTGATGTTTGCATTCAACTCA
count          : 13
rate           : 0.004095778198
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.945832253755
low_complexity : 0

rank           : 7
full_47bp      : ACAGAAGCATTCTCAGAAACTTCTTTGTGATGTGTGCATTCAACTCA
count          : 10
rate           : 0.003150598614
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.956171643341
low_complexity : 0

rank           : 8
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCATGATGTCTGCATTCAACTCA
count          : 7
rate           : 0.002205419030
length         : 47
gc             : 0.404255319149
at             : 0.595744680851
entropy_bits   : 1.932856316406
low_complexity : 0

rank           : 9
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCGTGGTGTCTGCATTCAACTCA
count          : 7
rate           : 0.002205419030
length         : 47
gc             : 0.446808510638
at             : 0.553191489362
entropy_bits   : 1.971009378203
low_complexity : 0

rank           : 10
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCGTGAGGTTTGCATTCAACTCA
count          : 7
rate           : 0.002205419030
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.971009378203
low_complexity : 0

rank           : 11
full_47bp      : ACAGAAGCATTCTCAGAACCCTCTTCGTGATGTCTGCATTCAACTCA
count          : 6
rate           : 0.001890359168
length         : 47
gc             : 0.446808510638
at             : 0.553191489362
entropy_bits   : 1.955314680723
low_complexity : 0

rank           : 12
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTGTGCATTCAACTCA
count          : 6
rate           : 0.001890359168
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.971009378203
low_complexity : 0

rank           : 13
full_47bp      : ACAGAAGCATTCTCAGAACCTCCTTCGTGATGTCTGCATTCAACTCA
count          : 5
rate           : 0.001575299307
length         : 47
gc             : 0.446808510638
at             : 0.553191489362
entropy_bits   : 1.955314680723
low_complexity : 0

rank           : 14
full_47bp      : ACAGAAGCATTCTCAGAACACTCTTCGTGATGTTTGCATTCAACTCA
count          : 5
rate           : 0.001575299307
length         : 47
gc             : 0.404255319149
at             : 0.595744680851
entropy_bits   : 1.952951143578
low_complexity : 0

rank           : 15
full_47bp      : ACAGAAGCATTCTCAGAAACTTCTTTGTGATGTTTGCATTCAACTCA
count          : 5
rate           : 0.001575299307
length         : 47
gc             : 0.361702127660
at             : 0.638297872340
entropy_bits   : 1.933871100357
low_complexity : 0

rank           : 16
full_47bp      : ACAGAAGCATTCTCAGAACTTTCTTCGTGATGTTTGCATTCAACTCA
count          : 4
rate           : 0.001260239445
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.941589945396
low_complexity : 0

rank           : 17
full_47bp      : ACAGAAGCATTCTCAGAATCCTCTTCGTGATGTTTGCATTCAACTCA
count          : 4
rate           : 0.001260239445
length         : 47
gc             : 0.404255319149
at             : 0.595744680851
entropy_bits   : 1.950756729699
low_complexity : 0

rank           : 18
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCGTGCTGTCTGCATTCAACTCA
count          : 4
rate           : 0.001260239445
length         : 47
gc             : 0.446808510638
at             : 0.553191489362
entropy_bits   : 1.952951143578
low_complexity : 0

rank           : 19
full_47bp      : ACAGAAGCATTCTCAGAATCTTCTTCGTGATGATTGCATTCAACTCA
count          : 4
rate           : 0.001260239445
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.945832253755
low_complexity : 0

rank           : 20
full_47bp      : ACAGAAGCATTCTCAGAACCCTCTTCCTGATGTTTGCATTCAACTCA
count          : 4
rate           : 0.001260239445
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.932856316406
low_complexity : 0

rank           : 21
full_47bp      : ACAGAAGCATTCTCAGAATCTTCTTCGTGATGTCTGCATTCAACTCA
count          : 4
rate           : 0.001260239445
length         : 47
gc             : 0.404255319149
at             : 0.595744680851
entropy_bits   : 1.950756729699
low_complexity : 0

rank           : 22
full_47bp      : ACAGAAGCATTCTCAGAAACTTCTTTGTGATGCTTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.945832253755
low_complexity : 0

rank           : 23
full_47bp      : ACAGAAGCATTCTCAGAACGTTCTTCGTGATGTCTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.971009378203
low_complexity : 0

rank           : 24
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCCTGATGTCTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.932856316406
low_complexity : 0

rank           : 25
full_47bp      : ACAGAAGCATTCTCAGAACCCTCTTGGTGATGTTTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.425531914894
at             : 0.574468085106
entropy_bits   : 1.971009378203
low_complexity : 0

rank           : 26
full_47bp      : ACAGAAGCATTCTCAGAACATTCTTCGTGATGTTTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.945832253755
low_complexity : 0

rank           : 27
full_47bp      : ACAGAAGCATTCTCAGAACCCTCTTCGTGACGTTTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.446808510638
at             : 0.553191489362
entropy_bits   : 1.955314680723
low_complexity : 0

rank           : 28
full_47bp      : ACAGAAGCATTCTCAGAACCTTCTTCTTGATGTCTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.404255319149
at             : 0.595744680851
entropy_bits   : 1.930661902526
low_complexity : 0

rank           : 29
full_47bp      : ACAGAAGCATTCTCAGAACCTACTTCGTGATGTTTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.404255319149
at             : 0.595744680851
entropy_bits   : 1.952951143578
low_complexity : 0

rank           : 30
full_47bp      : ACAGAAGCATTCTCAGAAACTACTTTGTGATGTGTGCATTCAACTCA
count          : 3
rate           : 0.000945179584
length         : 47
gc             : 0.382978723404
at             : 0.617021276596
entropy_bits   : 1.956171643341
low_complexity : 0



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v04_real\cluster_v04_delta_spacer.csv |
>>   Select-Object -First 40


rank                : 1
next_delta_family   : 170+0
spacer_16bp         : CCTTCTTCGTGATGTC
count               : 780
rate                : 0.245746691871
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 2
next_delta_family   : other
spacer_16bp         : CCCTCTTCGTGATGTT
count               : 602
rate                : 0.189666036547
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 3
next_delta_family   : 168-1
spacer_16bp         : CCCTCTTCGTGATGTT
count               : 363
rate                : 0.114366729679
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 4
next_delta_family   : 168+1
spacer_16bp         : CCTTCTTCGTGATGTC
count               : 155
rate                : 0.048834278513
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 5
next_delta_family   : 2379+0
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 138
rate                : 0.043478260870
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 6
next_delta_family   : other
spacer_16bp         : CCTTCTTCGTGATGTC
count               : 134
rate                : 0.042218021424
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 7
next_delta_family   : 2379-1
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 127
rate                : 0.040012602394
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 8
next_delta_family   : other
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 112
rate                : 0.035286704474
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 9
next_delta_family   : 2379+0
spacer_16bp         : ACTTTTCTGTGATGAC
count               : 74
rate                : 0.023314429742
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.880240814944

rank                : 10
next_delta_family   : 2380+0
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 64
rate                : 0.020163831128
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 11
next_delta_family   : 2379-2
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 45
rate                : 0.014177693762
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 12
next_delta_family   : 508+0
spacer_16bp         : GCTTCTCTGTGATGAC
count               : 44
rate                : 0.013862633900
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.905639062230

rank                : 13
next_delta_family   : 341-3
spacer_16bp         : CCTTCTTCGTGATGTC
count               : 44
rate                : 0.013862633900
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 14
next_delta_family   : 2379-3
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 43
rate                : 0.013547574039
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 15
next_delta_family   : other
spacer_16bp         : ACTTTTCTGTGATGAC
count               : 41
rate                : 0.012917454316
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.880240814944

rank                : 16
next_delta_family   : 2380+1
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 29
rate                : 0.009136735980
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 17
next_delta_family   : 2380+0
spacer_16bp         : ACTTTTCTGTGATGAC
count               : 24
rate                : 0.007561436673
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.880240814944

rank                : 18
next_delta_family   : 2379+0
spacer_16bp         : CCCTCTTCGTGATGTT
count               : 18
rate                : 0.005671077505
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 19
next_delta_family   : 2379-1
spacer_16bp         : CCCTCTTCGTGATGTT
count               : 13
rate                : 0.004095778198
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 20
next_delta_family   : 508+1
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 13
rate                : 0.004095778198
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 21
next_delta_family   : 2380+1
spacer_16bp         : ACTTTTCTGTGATGAC
count               : 11
rate                : 0.003465658475
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.880240814944

rank                : 22
next_delta_family   : 508+2
spacer_16bp         : ACTCCTTTGTGATGTT
count               : 11
rate                : 0.003465658475
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.780639062230

rank                : 23
next_delta_family   : 2380+0
spacer_16bp         : CCCTCTTCGTGATGTT
count               : 10
rate                : 0.003150598614
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 24
next_delta_family   : 2379-1
spacer_16bp         : ACTTTTCTGTGATGAC
count               : 9
rate                : 0.002835538752
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.880240814944

rank                : 25
next_delta_family   : 171+0
spacer_16bp         : CCTTCTTCGTGATGTC
count               : 9
rate                : 0.002835538752
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 26
next_delta_family   : 168+0
spacer_16bp         : CCCTCTTCGTGATGTT
count               : 8
rate                : 0.002520478891
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 27
next_delta_family   : 170+0
spacer_16bp         : CCTTCTTCGTGGTGTC
count               : 7
rate                : 0.002205419030
spacer_gc           : 0.562500000000
spacer_entropy_bits : 1.546179691947

rank                : 28
next_delta_family   : other
spacer_16bp         : ACTTCTTTGTGATGTG
count               : 7
rate                : 0.002205419030
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.750000000000

rank                : 29
next_delta_family   : 168-1
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 6
rate                : 0.001890359168
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 30
next_delta_family   : 2380+2
spacer_16bp         : ACTTTTCTGTGATGAC
count               : 5
rate                : 0.001575299307
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.880240814944

rank                : 31
next_delta_family   : 2380+2
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 5
rate                : 0.001575299307
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 32
next_delta_family   : other
spacer_16bp         : GCTTCTCTGTGATGAC
count               : 5
rate                : 0.001575299307
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.905639062230

rank                : 33
next_delta_family   : 170+0
spacer_16bp         : CCTCCTTCGTGATGTC
count               : 5
rate                : 0.001575299307
spacer_gc           : 0.562500000000
spacer_entropy_bits : 1.764097655574

rank                : 34
next_delta_family   : 168+0
spacer_16bp         : CCTTCTTCGTGATGTT
count               : 5
rate                : 0.001575299307
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.702819531115

rank                : 35
next_delta_family   : 168+1
spacer_16bp         : CCCTCTTCGTGATGTT
count               : 4
rate                : 0.001260239445
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 36
next_delta_family   : 168-1
spacer_16bp         : CACTCTTCGTGATGTT
count               : 4
rate                : 0.001260239445
spacer_gc           : 0.437500000000
spacer_entropy_bits : 1.849601752715

rank                : 37
next_delta_family   : 2380+0
spacer_16bp         : CCTTCTTCGTGATGTC
count               : 4
rate                : 0.001260239445
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.748999223062

rank                : 38
next_delta_family   : 507+0
spacer_16bp         : CCTTCTTCGTGAGGTT
count               : 4
rate                : 0.001260239445
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.771782221600

rank                : 39
next_delta_family   : 170+0
spacer_16bp         : CGTTCTTCGTGATGTC
count               : 3
rate                : 0.000945179584
spacer_gc           : 0.500000000000
spacer_entropy_bits : 1.771782221600

rank                : 40
next_delta_family   : other
spacer_16bp         : CTTTCTTCGTGATGTT
count               : 3
rate                : 0.000945179584
spacer_gc           : 0.375000000000
spacer_entropy_bits : 1.622556248918



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v04_real\cluster_v04_validation.csv

validation count rate
---------- ----- ----
ok         3174  1.000000000000


PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v04_real\* `
>>   -DestinationPath .\Decode_chr17_v04_real_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun>

PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_spacer_state_transition_v0_5.ps1 `
>>   -SequenceDir .\Decode_chr17_v04_real `
>>   -OutDir .\Decode_chr17_v05_real `
>>   -TopStates 12 `
>>   -NGram 3
KALYX spacer-state transition decoder complete: Decode_chr17_v05_real
  blocks=3174 states=92 transitions=3173
  report=Decode_chr17_v05_real\cluster_v05_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v05_real\cluster_v05_report.md
# KALYX Spacer-State Transition Decoder v0.5

## Boundary

Dieses Artefakt modelliert Spacer als Zustände und Blockabstände als Übergangskanten.
Es beweist keinen natürlichen oder künstlichen Ursprung. Ursprungsaussagen benötigen externe Kontrollen.

## Summary

- input blocks: `3174`
- unique spacer states: `92`
- raw transitions: `3173`
- aggregated transitions: `332`
- max same-state run length: `140`
- top state: `S001` spacer=`CCTTCTTCGTGATGTC` count=`1140` rate=`0.359168241966`
- top transition: `S001 --170+0--> S002` count=`729`

## Top states

| state | spacer | count | rate | dominant_window |
|---|---|---:|---:|---:|
| S001 | `CCTTCTTCGTGATGTC` | 1140 | 0.359168241966 | 23 |
| S002 | `CCCTCTTCGTGATGTT` | 1027 | 0.323566477631 | 23 |
| S003 | `CCTTCTTCGTGATGTT` | 588 | 0.185255198488 | 24 |
| S004 | `ACTTTTCTGTGATGAC` | 169 | 0.053245116572 | 21 |
| S005 | `GCTTCTCTGTGATGAC` | 52 | 0.016383112791 | 23 |
| S006 | `ACTCCTTTGTGATGTT` | 13 | 0.004095778198 | 25 |
| S007 | `ACTTCTTTGTGATGTG` | 10 | 0.003150598614 | 25 |
| S008 | `CCTTCTTCATGATGTC` | 7 | 0.002205419030 | 23 |
| S009 | `CCTTCTTCGTGGTGTC` | 7 | 0.002205419030 | 23 |
| S010 | `CCTTCTTCGTGAGGTT` | 7 | 0.002205419030 | 25 |
| S011 | `CCCTCTTCGTGATGTC` | 6 | 0.001890359168 | 22 |
| S012 | `CCTTCTTCGTGATGTG` | 6 | 0.001890359168 | 24 |
| S013 | `CCTCCTTCGTGATGTC` | 5 | 0.001575299307 | 22 |
| S014 | `CACTCTTCGTGATGTT` | 5 | 0.001575299307 | 24 |
| S015 | `ACTTCTTTGTGATGTT` | 5 | 0.001575299307 | 25 |
| S016 | `CTTTCTTCGTGATGTT` | 4 | 0.001260239445 | 22 |
| S017 | `TCCTCTTCGTGATGTT` | 4 | 0.001260239445 | 22 |
| S018 | `CCTTCTTCGTGCTGTC` | 4 | 0.001260239445 | 22 |
| S019 | `TCTTCTTCGTGATGAT` | 4 | 0.001260239445 | 23 |
| S020 | `CCCTCTTCCTGATGTT` | 4 | 0.001260239445 | 23 |

## Top transitions

| rank | transition | delta_family | count | rate_all | conditional_rate |
|---:|---|---|---:|---:|---:|
| 1 | `S001→S002` | `170+0` | 729 | 0.229751024267 | 0.934615384615 |
| 2 | `S002→S001` | `other` | 481 | 0.151591553735 | 0.799003322259 |
| 3 | `S002→S003` | `168-1` | 347 | 0.109360226915 | 0.955922865014 |
| 4 | `S001→S002` | `168+1` | 149 | 0.046958714151 | 0.961290322581 |
| 5 | `S003→S001` | `2379+0` | 131 | 0.041285849354 | 0.949275362319 |
| 6 | `S003→S001` | `2379-1` | 121 | 0.038134257800 | 0.952755905512 |
| 7 | `S004→S004` | `2379+0` | 74 | 0.023321777498 | 1.000000000000 |
| 8 | `S001→S001` | `other` | 63 | 0.019855026789 | 0.470149253731 |
| 9 | `S003→S001` | `2380+0` | 59 | 0.018594390167 | 0.921875000000 |
| 10 | `S001→S003` | `other` | 54 | 0.017018594390 | 0.402985074627 |
| 11 | `S003→S001` | `2379-2` | 43 | 0.013551843681 | 0.955555555556 |
| 12 | `S005→S001` | `508+0` | 42 | 0.013236684526 | 0.954545454545 |
| 13 | `S003→S001` | `2379-3` | 42 | 0.013236684526 | 0.976744186047 |
| 14 | `S001→S003` | `341-3` | 40 | 0.012606366215 | 0.909090909091 |
| 15 | `S003→S002` | `other` | 38 | 0.011976047904 | 0.339285714286 |
| 16 | `S002→S002` | `other` | 36 | 0.011345729593 | 0.059800664452 |
| 17 | `S002→S003` | `other` | 32 | 0.010085092972 | 0.053156146179 |
| 18 | `S003→S001` | `other` | 31 | 0.009769933817 | 0.276785714286 |
| 19 | `S003→S001` | `2380+1` | 26 | 0.008194138040 | 0.896551724138 |
| 20 | `S004→S004` | `2380+0` | 24 | 0.007563819729 | 1.000000000000 |
| 21 | `S002→S005` | `other` | 24 | 0.007563819729 | 0.039867109635 |
| 22 | `S003→S005` | `other` | 22 | 0.006933501418 | 0.196428571429 |
| 23 | `S004→S004` | `other` | 20 | 0.006303183107 | 0.487804878049 |
| 24 | `S001→S003` | `170+0` | 20 | 0.006303183107 | 0.025641025641 |
| 25 | `S002→S001` | `2379+0` | 17 | 0.005357705641 | 0.944444444444 |
| 26 | `S003→S004` | `508+1` | 13 | 0.004097069020 | 1.000000000000 |
| 27 | `S003→S003` | `other` | 12 | 0.003781909864 | 0.107142857143 |
| 28 | `S004→S004` | `2380+1` | 10 | 0.003151591554 | 0.909090909091 |
| 29 | `S002→S001` | `2379-1` | 10 | 0.003151591554 | 0.769230769231 |
| 30 | `S004→S004` | `2379-1` | 9 | 0.002836432398 | 1.000000000000 |

## Top run lengths

| state | run_length | count |
|---|---:|---:|
| S001 | 1 | 995 |
| S002 | 1 | 937 |
| S003 | 1 | 548 |
| S001 | 2 | 65 |
| S005 | 1 | 52 |
| S002 | 2 | 42 |
| S003 | 2 | 20 |
| S004 | 1 | 19 |
| S006 | 1 | 11 |
| S008 | 1 | 7 |
| S009 | 1 | 7 |
| S010 | 1 | 7 |
| S011 | 1 | 6 |
| S012 | 1 | 6 |
| S007 | 1 | 6 |
| S013 | 1 | 5 |
| S014 | 1 | 5 |
| S001 | 3 | 5 |
| S015 | 1 | 5 |
| S016 | 1 | 4 |
| S017 | 1 | 4 |
| S018 | 1 | 4 |
| S019 | 1 | 4 |
| S020 | 1 | 4 |
| S021 | 1 | 4 |
| S022 | 1 | 3 |
| S023 | 1 | 3 |
| S024 | 1 | 3 |
| S025 | 1 | 3 |
| S026 | 1 | 3 |

## Output files

```text
cluster_v05_states.csv
cluster_v05_block_states.csv
cluster_v05_transitions_raw.csv
cluster_v05_transitions.csv
cluster_v05_transition_matrix.csv
cluster_v05_state_delta_matrix.csv
cluster_v05_runs.csv
cluster_v05_run_lengths.csv
cluster_v05_state_ngrams_n3.csv
cluster_v05_manifest.json
cluster_v05_report.md
```

## Interpretation guardrail

Wenn einzelne Übergänge hohe bedingte Wahrscheinlichkeiten haben, ist die Spacer-Folge geordnet.
Wenn Run-Längen und N-Gramme stark konzentriert sind, entsteht ein echter Zustandsautomat.
Erst nach RepeatMasker-/GC-/Cross-Chromosom-Kontrollen darf man über Ursprung sprechen.
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v05_real\cluster_v05_states.csv |
>>   Select-Object -First 20


state_id                 : S001
rank                     : 1
spacer_16bp              : CCTTCTTCGTGATGTC
count                    : 1140
rate                     : 0.359168241966
gc                       : 0.500000000000
entropy_bits             : 1.748999223062
hamming_to_top_consensus : 0
dominant_window          : 23
dominant_window_count    : 362
first_start0             : 23201468
last_start0              : 26562554
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTCTGCATTCAACTCA
dominant_full47_count    : 1140

state_id                 : S002
rank                     : 2
spacer_16bp              : CCCTCTTCGTGATGTT
count                    : 1027
rate                     : 0.323566477631
gc                       : 0.500000000000
entropy_bits             : 1.748999223062
hamming_to_top_consensus : 2
dominant_window          : 23
dominant_window_count    : 356
first_start0             : 23199425
last_start0              : 26565443
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCCTCTTCGTGATGTTTGCATTCAACTCA
dominant_full47_count    : 1027

state_id                 : S003
rank                     : 3
spacer_16bp              : CCTTCTTCGTGATGTT
count                    : 588
rate                     : 0.185255198488
gc                       : 0.437500000000
entropy_bits             : 1.702819531115
hamming_to_top_consensus : 1
dominant_window          : 24
dominant_window_count    : 199
first_start0             : 22815062
last_start0              : 26629334
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTTTGCATTCAACTCA
dominant_full47_count    : 588

state_id                 : S004
rank                     : 4
spacer_16bp              : ACTTTTCTGTGATGAC
count                    : 169
rate                     : 0.053245116572
gc                       : 0.375000000000
entropy_bits             : 1.880240814944
hamming_to_top_consensus : 5
dominant_window          : 21
dominant_window_count    : 99
first_start0             : 22747231
last_start0              : 26627634
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAAACTTTTCTGTGATGACTGCATTCAACTCA
dominant_full47_count    : 169

state_id                 : S005
rank                     : 5
spacer_16bp              : GCTTCTCTGTGATGAC
count                    : 52
rate                     : 0.016383112791
gc                       : 0.500000000000
entropy_bits             : 1.905639062230
hamming_to_top_consensus : 4
dominant_window          : 23
dominant_window_count    : 20
first_start0             : 23214378
last_start0              : 26603738
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAAGCTTCTCTGTGATGACTGCATTCAACTCA
dominant_full47_count    : 52

state_id                 : S006
rank                     : 6
spacer_16bp              : ACTCCTTTGTGATGTT
count                    : 13
rate                     : 0.004095778198
gc                       : 0.375000000000
entropy_bits             : 1.780639062230
hamming_to_top_consensus : 4
dominant_window          : 25
dominant_window_count    : 13
first_start0             : 26567921
last_start0              : 26622394
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAAACTCCTTTGTGATGTTTGCATTCAACTCA
dominant_full47_count    : 13

state_id                 : S007
rank                     : 7
spacer_16bp              : ACTTCTTTGTGATGTG
count                    : 10
rate                     : 0.003150598614
gc                       : 0.375000000000
entropy_bits             : 1.750000000000
hamming_to_top_consensus : 3
dominant_window          : 25
dominant_window_count    : 10
first_start0             : 26644071
last_start0              : 26710686
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAAACTTCTTTGTGATGTGTGCATTCAACTCA
dominant_full47_count    : 10

state_id                 : S008
rank                     : 8
spacer_16bp              : CCTTCTTCATGATGTC
count                    : 7
rate                     : 0.002205419030
gc                       : 0.437500000000
entropy_bits             : 1.796179691947
hamming_to_top_consensus : 1
dominant_window          : 23
dominant_window_count    : 3
first_start0             : 23910129
last_start0              : 25626596
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTTCTTCATGATGTCTGCATTCAACTCA
dominant_full47_count    : 7

state_id                 : S009
rank                     : 9
spacer_16bp              : CCTTCTTCGTGGTGTC
count                    : 7
rate                     : 0.002205419030
gc                       : 0.562500000000
entropy_bits             : 1.546179691947
hamming_to_top_consensus : 1
dominant_window          : 23
dominant_window_count    : 3
first_start0             : 24053317
last_start0              : 26521981
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTTCTTCGTGGTGTCTGCATTCAACTCA
dominant_full47_count    : 7

state_id                 : S010
rank                     : 10
spacer_16bp              : CCTTCTTCGTGAGGTT
count                    : 7
rate                     : 0.002205419030
gc                       : 0.500000000000
entropy_bits             : 1.771782221600
hamming_to_top_consensus : 2
dominant_window          : 25
dominant_window_count    : 7
first_start0             : 26585322
last_start0              : 26622904
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTTCTTCGTGAGGTTTGCATTCAACTCA
dominant_full47_count    : 7

state_id                 : S011
rank                     : 11
spacer_16bp              : CCCTCTTCGTGATGTC
count                    : 6
rate                     : 0.001890359168
gc                       : 0.562500000000
entropy_bits             : 1.764097655574
hamming_to_top_consensus : 1
dominant_window          : 22
dominant_window_count    : 2
first_start0             : 23219810
last_start0              : 26382124
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCCTCTTCGTGATGTCTGCATTCAACTCA
dominant_full47_count    : 6

state_id                 : S012
rank                     : 12
spacer_16bp              : CCTTCTTCGTGATGTG
count                    : 6
rate                     : 0.001890359168
gc                       : 0.500000000000
entropy_bits             : 1.771782221600
hamming_to_top_consensus : 1
dominant_window          : 24
dominant_window_count    : 3
first_start0             : 23778980
last_start0              : 26430154
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTTCTTCGTGATGTGTGCATTCAACTCA
dominant_full47_count    : 6

state_id                 : S013
rank                     : 13
spacer_16bp              : CCTCCTTCGTGATGTC
count                    : 5
rate                     : 0.001575299307
gc                       : 0.562500000000
entropy_bits             : 1.764097655574
hamming_to_top_consensus : 1
dominant_window          : 22
dominant_window_count    : 2
first_start0             : 23848871
last_start0              : 26147405
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTCCTTCGTGATGTCTGCATTCAACTCA
dominant_full47_count    : 5

state_id                 : S014
rank                     : 14
spacer_16bp              : CACTCTTCGTGATGTT
count                    : 5
rate                     : 0.001575299307
gc                       : 0.437500000000
entropy_bits             : 1.849601752715
hamming_to_top_consensus : 3
dominant_window          : 24
dominant_window_count    : 2
first_start0             : 23915560
last_start0              : 26288409
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACACTCTTCGTGATGTTTGCATTCAACTCA
dominant_full47_count    : 5

state_id                 : S015
rank                     : 15
spacer_16bp              : ACTTCTTTGTGATGTT
count                    : 5
rate                     : 0.001575299307
gc                       : 0.312500000000
entropy_bits             : 1.669736717803
hamming_to_top_consensus : 3
dominant_window          : 25
dominant_window_count    : 5
first_start0             : 26584813
last_start0              : 26614629
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAAACTTCTTTGTGATGTTTGCATTCAACTCA
dominant_full47_count    : 5

state_id                 : S016
rank                     : 16
spacer_16bp              : CTTTCTTCGTGATGTT
count                    : 4
rate                     : 0.001260239445
gc                       : 0.375000000000
entropy_bits             : 1.622556248918
hamming_to_top_consensus : 2
dominant_window          : 22
dominant_window_count    : 2
first_start0             : 23409151
last_start0              : 24635663
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACTTTCTTCGTGATGTTTGCATTCAACTCA
dominant_full47_count    : 4

state_id                 : S017
rank                     : 17
spacer_16bp              : TCCTCTTCGTGATGTT
count                    : 4
rate                     : 0.001260239445
gc                       : 0.437500000000
entropy_bits             : 1.702819531115
hamming_to_top_consensus : 3
dominant_window          : 22
dominant_window_count    : 2
first_start0             : 23513821
last_start0              : 25094492
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAATCCTCTTCGTGATGTTTGCATTCAACTCA
dominant_full47_count    : 4

state_id                 : S018
rank                     : 18
spacer_16bp              : CCTTCTTCGTGCTGTC
count                    : 4
rate                     : 0.001260239445
gc                       : 0.562500000000
entropy_bits             : 1.505240814944
hamming_to_top_consensus : 1
dominant_window          : 22
dominant_window_count    : 1
first_start0             : 23788820
last_start0              : 26244973
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCTTCTTCGTGCTGTCTGCATTCAACTCA
dominant_full47_count    : 4

state_id                 : S019
rank                     : 19
spacer_16bp              : TCTTCTTCGTGATGAT
count                    : 4
rate                     : 0.001260239445
gc                       : 0.375000000000
entropy_bits             : 1.780639062230
hamming_to_top_consensus : 3
dominant_window          : 23
dominant_window_count    : 2
first_start0             : 23815107
last_start0              : 25327785
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAATCTTCTTCGTGATGATTGCATTCAACTCA
dominant_full47_count    : 4

state_id                 : S020
rank                     : 20
spacer_16bp              : CCCTCTTCCTGATGTT
count                    : 4
rate                     : 0.001260239445
gc                       : 0.500000000000
entropy_bits             : 1.677421283829
hamming_to_top_consensus : 3
dominant_window          : 23
dominant_window_count    : 2
first_start0             : 24306955
last_start0              : 25725254
full47_variants          : 1
dominant_full47          : ACAGAAGCATTCTCAGAACCCTCTTCCTGATGTTTGCATTCAACTCA
dominant_full47_count    : 4



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v05_real\cluster_v05_transitions.csv |
>>   Select-Object -First 40


rank                         : 1
from_state_id                : S001
to_state_id                  : S002
delta_family                 : 170+0
count                        : 729
rate_all                     : 0.229751024267
rate_within_from_state_delta : 0.934615384615

rank                         : 2
from_state_id                : S002
to_state_id                  : S001
delta_family                 : other
count                        : 481
rate_all                     : 0.151591553735
rate_within_from_state_delta : 0.799003322259

rank                         : 3
from_state_id                : S002
to_state_id                  : S003
delta_family                 : 168-1
count                        : 347
rate_all                     : 0.109360226915
rate_within_from_state_delta : 0.955922865014

rank                         : 4
from_state_id                : S001
to_state_id                  : S002
delta_family                 : 168+1
count                        : 149
rate_all                     : 0.046958714151
rate_within_from_state_delta : 0.961290322581

rank                         : 5
from_state_id                : S003
to_state_id                  : S001
delta_family                 : 2379+0
count                        : 131
rate_all                     : 0.041285849354
rate_within_from_state_delta : 0.949275362319

rank                         : 6
from_state_id                : S003
to_state_id                  : S001
delta_family                 : 2379-1
count                        : 121
rate_all                     : 0.038134257800
rate_within_from_state_delta : 0.952755905512

rank                         : 7
from_state_id                : S004
to_state_id                  : S004
delta_family                 : 2379+0
count                        : 74
rate_all                     : 0.023321777498
rate_within_from_state_delta : 1.000000000000

rank                         : 8
from_state_id                : S001
to_state_id                  : S001
delta_family                 : other
count                        : 63
rate_all                     : 0.019855026789
rate_within_from_state_delta : 0.470149253731

rank                         : 9
from_state_id                : S003
to_state_id                  : S001
delta_family                 : 2380+0
count                        : 59
rate_all                     : 0.018594390167
rate_within_from_state_delta : 0.921875000000

rank                         : 10
from_state_id                : S001
to_state_id                  : S003
delta_family                 : other
count                        : 54
rate_all                     : 0.017018594390
rate_within_from_state_delta : 0.402985074627

rank                         : 11
from_state_id                : S003
to_state_id                  : S001
delta_family                 : 2379-2
count                        : 43
rate_all                     : 0.013551843681
rate_within_from_state_delta : 0.955555555556

rank                         : 12
from_state_id                : S005
to_state_id                  : S001
delta_family                 : 508+0
count                        : 42
rate_all                     : 0.013236684526
rate_within_from_state_delta : 0.954545454545

rank                         : 13
from_state_id                : S003
to_state_id                  : S001
delta_family                 : 2379-3
count                        : 42
rate_all                     : 0.013236684526
rate_within_from_state_delta : 0.976744186047

rank                         : 14
from_state_id                : S001
to_state_id                  : S003
delta_family                 : 341-3
count                        : 40
rate_all                     : 0.012606366215
rate_within_from_state_delta : 0.909090909091

rank                         : 15
from_state_id                : S003
to_state_id                  : S002
delta_family                 : other
count                        : 38
rate_all                     : 0.011976047904
rate_within_from_state_delta : 0.339285714286

rank                         : 16
from_state_id                : S002
to_state_id                  : S002
delta_family                 : other
count                        : 36
rate_all                     : 0.011345729593
rate_within_from_state_delta : 0.059800664452

rank                         : 17
from_state_id                : S002
to_state_id                  : S003
delta_family                 : other
count                        : 32
rate_all                     : 0.010085092972
rate_within_from_state_delta : 0.053156146179

rank                         : 18
from_state_id                : S003
to_state_id                  : S001
delta_family                 : other
count                        : 31
rate_all                     : 0.009769933817
rate_within_from_state_delta : 0.276785714286

rank                         : 19
from_state_id                : S003
to_state_id                  : S001
delta_family                 : 2380+1
count                        : 26
rate_all                     : 0.008194138040
rate_within_from_state_delta : 0.896551724138

rank                         : 20
from_state_id                : S004
to_state_id                  : S004
delta_family                 : 2380+0
count                        : 24
rate_all                     : 0.007563819729
rate_within_from_state_delta : 1.000000000000

rank                         : 21
from_state_id                : S002
to_state_id                  : S005
delta_family                 : other
count                        : 24
rate_all                     : 0.007563819729
rate_within_from_state_delta : 0.039867109635

rank                         : 22
from_state_id                : S003
to_state_id                  : S005
delta_family                 : other
count                        : 22
rate_all                     : 0.006933501418
rate_within_from_state_delta : 0.196428571429

rank                         : 23
from_state_id                : S004
to_state_id                  : S004
delta_family                 : other
count                        : 20
rate_all                     : 0.006303183107
rate_within_from_state_delta : 0.487804878049

rank                         : 24
from_state_id                : S001
to_state_id                  : S003
delta_family                 : 170+0
count                        : 20
rate_all                     : 0.006303183107
rate_within_from_state_delta : 0.025641025641

rank                         : 25
from_state_id                : S002
to_state_id                  : S001
delta_family                 : 2379+0
count                        : 17
rate_all                     : 0.005357705641
rate_within_from_state_delta : 0.944444444444

rank                         : 26
from_state_id                : S003
to_state_id                  : S004
delta_family                 : 508+1
count                        : 13
rate_all                     : 0.004097069020
rate_within_from_state_delta : 1.000000000000

rank                         : 27
from_state_id                : S003
to_state_id                  : S003
delta_family                 : other
count                        : 12
rate_all                     : 0.003781909864
rate_within_from_state_delta : 0.107142857143

rank                         : 28
from_state_id                : S004
to_state_id                  : S004
delta_family                 : 2380+1
count                        : 10
rate_all                     : 0.003151591554
rate_within_from_state_delta : 0.909090909091

rank                         : 29
from_state_id                : S002
to_state_id                  : S001
delta_family                 : 2379-1
count                        : 10
rate_all                     : 0.003151591554
rate_within_from_state_delta : 0.769230769231

rank                         : 30
from_state_id                : S004
to_state_id                  : S004
delta_family                 : 2379-1
count                        : 9
rate_all                     : 0.002836432398
rate_within_from_state_delta : 1.000000000000

rank                         : 31
from_state_id                : S004
to_state_id                  : S006
delta_family                 : other
count                        : 9
rate_all                     : 0.002836432398
rate_within_from_state_delta : 0.219512195122

rank                         : 32
from_state_id                : S002
to_state_id                  : S001
delta_family                 : 2380+0
count                        : 8
rate_all                     : 0.002521273243
rate_within_from_state_delta : 0.800000000000

rank                         : 33
from_state_id                : S001
to_state_id                  : S002
delta_family                 : 171+0
count                        : 8
rate_all                     : 0.002521273243
rate_within_from_state_delta : 0.888888888889

rank                         : 34
from_state_id                : S002
to_state_id                  : S003
delta_family                 : 168+0
count                        : 8
rate_all                     : 0.002521273243
rate_within_from_state_delta : 1.000000000000

rank                         : 35
from_state_id                : S006
to_state_id                  : S003
delta_family                 : 508+2
count                        : 8
rate_all                     : 0.002521273243
rate_within_from_state_delta : 0.727272727273

rank                         : 36
from_state_id                : S002
to_state_id                  : S002
delta_family                 : 168-1
count                        : 7
rate_all                     : 0.002206114088
rate_within_from_state_delta : 0.019283746556

rank                         : 37
from_state_id                : S009
to_state_id                  : S002
delta_family                 : 170+0
count                        : 7
rate_all                     : 0.002206114088
rate_within_from_state_delta : 1.000000000000

rank                         : 38
from_state_id                : S004
to_state_id                  : S003
delta_family                 : other
count                        : 5
rate_all                     : 0.001575795777
rate_within_from_state_delta : 0.121951219512

rank                         : 39
from_state_id                : S004
to_state_id                  : S004
delta_family                 : 2380+2
count                        : 5
rate_all                     : 0.001575795777
rate_within_from_state_delta : 1.000000000000

rank                         : 40
from_state_id                : S003
to_state_id                  : S001
delta_family                 : 2380+2
count                        : 5
rate_all                     : 0.001575795777
rate_within_from_state_delta : 1.000000000000



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v05_real\cluster_v05_state_delta_matrix.csv |
>>   Select-Object -First 40


rank              : 1
state_id          : S001
delta_family      : 170+0
count             : 780
rate_within_state : 0.684210526316

rank              : 2
state_id          : S002
delta_family      : other
count             : 602
rate_within_state : 0.586173320351

rank              : 3
state_id          : S002
delta_family      : 168-1
count             : 363
rate_within_state : 0.353456669912

rank              : 4
state_id          : S001
delta_family      : 168+1
count             : 155
rate_within_state : 0.135964912281

rank              : 5
state_id          : S003
delta_family      : 2379+0
count             : 138
rate_within_state : 0.234693877551

rank              : 6
state_id          : S001
delta_family      : other
count             : 134
rate_within_state : 0.117543859649

rank              : 7
state_id          : S003
delta_family      : 2379-1
count             : 127
rate_within_state : 0.215986394558

rank              : 8
state_id          : S003
delta_family      : other
count             : 112
rate_within_state : 0.190476190476

rank              : 9
state_id          : S004
delta_family      : 2379+0
count             : 74
rate_within_state : 0.437869822485

rank              : 10
state_id          : S003
delta_family      : 2380+0
count             : 64
rate_within_state : 0.108843537415

rank              : 11
state_id          : S003
delta_family      : 2379-2
count             : 45
rate_within_state : 0.076530612245

rank              : 12
state_id          : S005
delta_family      : 508+0
count             : 44
rate_within_state : 0.846153846154

rank              : 13
state_id          : S001
delta_family      : 341-3
count             : 44
rate_within_state : 0.038596491228

rank              : 14
state_id          : S003
delta_family      : 2379-3
count             : 43
rate_within_state : 0.073129251701

rank              : 15
state_id          : S004
delta_family      : other
count             : 41
rate_within_state : 0.242603550296

rank              : 16
state_id          : S003
delta_family      : 2380+1
count             : 29
rate_within_state : 0.049319727891

rank              : 17
state_id          : S004
delta_family      : 2380+0
count             : 24
rate_within_state : 0.142011834320

rank              : 18
state_id          : S002
delta_family      : 2379+0
count             : 18
rate_within_state : 0.017526777020

rank              : 19
state_id          : S002
delta_family      : 2379-1
count             : 13
rate_within_state : 0.012658227848

rank              : 20
state_id          : S003
delta_family      : 508+1
count             : 13
rate_within_state : 0.022108843537

rank              : 21
state_id          : S004
delta_family      : 2380+1
count             : 11
rate_within_state : 0.065088757396

rank              : 22
state_id          : S006
delta_family      : 508+2
count             : 11
rate_within_state : 0.846153846154

rank              : 23
state_id          : S002
delta_family      : 2380+0
count             : 10
rate_within_state : 0.009737098345

rank              : 24
state_id          : S004
delta_family      : 2379-1
count             : 9
rate_within_state : 0.053254437870

rank              : 25
state_id          : S001
delta_family      : 171+0
count             : 9
rate_within_state : 0.007894736842

rank              : 26
state_id          : S002
delta_family      : 168+0
count             : 8
rate_within_state : 0.007789678676

rank              : 27
state_id          : S009
delta_family      : 170+0
count             : 7
rate_within_state : 1.000000000000

rank              : 28
state_id          : S007
delta_family      : other
count             : 7
rate_within_state : 0.700000000000

rank              : 29
state_id          : S003
delta_family      : 168-1
count             : 6
rate_within_state : 0.010204081633

rank              : 30
state_id          : S004
delta_family      : 2380+2
count             : 5
rate_within_state : 0.029585798817

rank              : 31
state_id          : S003
delta_family      : 2380+2
count             : 5
rate_within_state : 0.008503401361

rank              : 32
state_id          : S005
delta_family      : other
count             : 5
rate_within_state : 0.096153846154

rank              : 33
state_id          : S013
delta_family      : 170+0
count             : 5
rate_within_state : 1.000000000000

rank              : 34
state_id          : S003
delta_family      : 168+0
count             : 5
rate_within_state : 0.008503401361

rank              : 35
state_id          : S002
delta_family      : 168+1
count             : 4
rate_within_state : 0.003894839338

rank              : 36
state_id          : S014
delta_family      : 168-1
count             : 4
rate_within_state : 0.800000000000

rank              : 37
state_id          : S001
delta_family      : 2380+0
count             : 4
rate_within_state : 0.003508771930

rank              : 38
state_id          : S010
delta_family      : 507+0
count             : 4
rate_within_state : 0.571428571429

rank              : 39
state_id          : S023
delta_family      : 170+0
count             : 3
rate_within_state : 1.000000000000

rank              : 40
state_id          : S016
delta_family      : other
count             : 3
rate_within_state : 0.750000000000



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v05_real\cluster_v05_run_lengths.csv |
>>   Select-Object -First 40

state_id run_length count
-------- ---------- -----
S001     1          995
S002     1          937
S003     1          548
S001     2          65
S005     1          52
S002     2          42
S003     2          20
S004     1          19
S006     1          11
S008     1          7
S009     1          7
S010     1          7
S011     1          6
S012     1          6
S007     1          6
S013     1          5
S014     1          5
S001     3          5
S015     1          5
S016     1          4
S017     1          4
S018     1          4
S019     1          4
S020     1          4
S021     1          4
S022     1          3
S023     1          3
S024     1          3
S025     1          3
S026     1          3
S027     1          3
S028     1          3
S029     1          3
S030     1          3
S002     3          2
S031     1          2
S032     1          2
S033     1          2
S034     1          2
S035     1          2


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v05_real\cluster_v05_state_ngrams_n3.csv |
>>   Select-Object -First 40


rank        : 1
n           : 3
state_ngram : S001→S002→S001
count       : 466
rate        : 0.146910466583

rank        : 2
n           : 3
state_ngram : S002→S001→S002
count       : 424
rate        : 0.133669609079

rank        : 3
n           : 3
state_ngram : S003→S001→S002
count       : 346
rate        : 0.109079445145

rank        : 4
n           : 3
state_ngram : S001→S002→S003
count       : 339
rate        : 0.106872635561

rank        : 5
n           : 3
state_ngram : S002→S003→S001
count       : 321
rate        : 0.101197982346

rank        : 6
n           : 3
state_ngram : S004→S004→S004
count       : 142
rate        : 0.044766708701

rank        : 7
n           : 3
state_ngram : S001→S003→S001
count       : 93
rate        : 0.029319041614

rank        : 8
n           : 3
state_ngram : S003→S001→S003
count       : 62
rate        : 0.019546027743

rank        : 9
n           : 3
state_ngram : S001→S001→S002
count       : 55
rate        : 0.017339218159

rank        : 10
n           : 3
state_ngram : S002→S001→S003
count       : 43
rate        : 0.013556116015

rank        : 11
n           : 3
state_ngram : S001→S002→S002
count       : 39
rate        : 0.012295081967

rank        : 12
n           : 3
state_ngram : S005→S001→S002
count       : 38
rate        : 0.011979823455

rank        : 13
n           : 3
state_ngram : S002→S001→S001
count       : 36
rate        : 0.011349306431

rank        : 14
n           : 3
state_ngram : S003→S001→S001
count       : 31
rate        : 0.009773013871

rank        : 15
n           : 3
state_ngram : S002→S003→S002
count       : 29
rate        : 0.009142496847

rank        : 16
n           : 3
state_ngram : S003→S002→S003
count       : 22
rate        : 0.006935687264

rank        : 17
n           : 3
state_ngram : S003→S005→S001
count       : 21
rate        : 0.006620428752

rank        : 18
n           : 3
state_ngram : S002→S005→S001
count       : 20
rate        : 0.006305170240

rank        : 19
n           : 3
state_ngram : S002→S002→S003
count       : 19
rate        : 0.005989911728

rank        : 20
n           : 3
state_ngram : S001→S002→S005
count       : 17
rate        : 0.005359394704

rank        : 21
n           : 3
state_ngram : S002→S002→S001
count       : 17
rate        : 0.005359394704

rank        : 22
n           : 3
state_ngram : S003→S002→S001
count       : 17
rate        : 0.005359394704

rank        : 23
n           : 3
state_ngram : S003→S003→S001
count       : 16
rate        : 0.005044136192

rank        : 24
n           : 3
state_ngram : S002→S003→S005
count       : 15
rate        : 0.004728877680

rank        : 25
n           : 3
state_ngram : S002→S003→S003
count       : 12
rate        : 0.003783102144

rank        : 26
n           : 3
state_ngram : S001→S003→S002
count       : 10
rate        : 0.003152585120

rank        : 27
n           : 3
state_ngram : S001→S001→S003
count       : 10
rate        : 0.003152585120

rank        : 28
n           : 3
state_ngram : S006→S003→S004
count       : 8
rate        : 0.002522068096

rank        : 29
n           : 3
state_ngram : S003→S004→S006
count       : 8
rate        : 0.002522068096

rank        : 30
n           : 3
state_ngram : S004→S003→S004
count       : 5
rate        : 0.001576292560

rank        : 31
n           : 3
state_ngram : S001→S003→S003
count       : 5
rate        : 0.001576292560

rank        : 32
n           : 3
state_ngram : S001→S002→S008
count       : 5
rate        : 0.001576292560

rank        : 33
n           : 3
state_ngram : S002→S002→S005
count       : 5
rate        : 0.001576292560

rank        : 34
n           : 3
state_ngram : S003→S009→S002
count       : 5
rate        : 0.001576292560

rank        : 35
n           : 3
state_ngram : S001→S001→S001
count       : 5
rate        : 0.001576292560

rank        : 36
n           : 3
state_ngram : S001→S003→S005
count       : 5
rate        : 0.001576292560

rank        : 37
n           : 3
state_ngram : S004→S006→S003
count       : 5
rate        : 0.001576292560

rank        : 38
n           : 3
state_ngram : S001→S002→S012
count       : 4
rate        : 0.001261034048

rank        : 39
n           : 3
state_ngram : S014→S003→S001
count       : 4
rate        : 0.001261034048

rank        : 40
n           : 3
state_ngram : S009→S002→S003
count       : 4
rate        : 0.001261034048



PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v05_real\* `
>>   -DestinationPath .\Decode_chr17_v05_real_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun>

PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_spacer_state_determinism_v0_6.ps1 `
>>   -StateDir .\Decode_chr17_v05_real `
>>   -OutDir .\Decode_chr17_v06_real `
>>   -Iterations 1000 `
>>   -MinGroupCount 10 `
>>   -NGram 3
KALYX spacer-state determinism v0.6 complete: Decode_chr17_v06_real
  transitions=3173 states=92 iterations=1000
  report=Decode_chr17_v06_real\cluster_v06_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v06_real\cluster_v06_report.md
# KALYX Spacer-State Determinism / Null Model Decoder v0.6

## Boundary

Dieses Artefakt testet die v0.5-Zustandsmaschine gegen interne Nullmodelle.
Es beweist keinen natürlichen oder künstlichen Ursprung. Es quantifiziert nur,
ob die Übergänge stärker geordnet sind als eine zustandshäufigkeits-erhaltende
Shuffle-Baseline.

## Summary

- transitions: `3173`
- blocks: `3174`
- states: `92`
- null iterations: `1000`
- ngram: `3`

## Null-model metrics

| metric | observed | null_mean | null_sd | z | p_empirical_ge |
|---|---:|---:|---:|---:|---:|
| `weighted_top1_accuracy` | 0.863220926568 | 0.400930980145 | 0.004065263392 | 113.717095759684 | 0.000999000999 |
| `mean_group_top1_accuracy` | 0.922785250163 | 0.800243225363 | 0.007892642919 | 15.526107800818 | 0.000999000999 |
| `mean_group_entropy_bits` | 0.236103541660 | 0.572824050236 | 0.017506115034 | -19.234450814760 | 1.000000000000 |
| `top_transition_count` | 729.000000000000 | 281.126000000000 | 10.583200083151 | 42.319335974102 | 0.000999000999 |
| `mutual_information_to_from_delta_bits` | 1.700763070072 | 0.298050521390 | 0.011043853336 | 127.012964222917 | 0.000999000999 |
| `max_same_state_run_length` | 140.000000000000 | 7.823000000000 | 1.206511914570 | 109.553000184900 | 0.000999000999 |
| `top_state_ngram_n3_count` | 466.000000000000 | 150.318000000000 | 8.942979145676 | 35.299422581414 | 0.000999000999 |

## Top group determinism

| rank | from | delta_family | n | top_to | top_count | top_rate | alternatives |
|---:|---|---|---:|---|---:|---:|---:|
| 1 | `S001` | `170+0` | 780 | `S002` | 729 | 0.934615384615 | 23 |
| 2 | `S002` | `other` | 602 | `S001` | 481 | 0.799003322259 | 25 |
| 3 | `S002` | `168-1` | 363 | `S003` | 347 | 0.955922865014 | 9 |
| 4 | `S001` | `168+1` | 155 | `S002` | 149 | 0.961290322581 | 6 |
| 5 | `S003` | `2379+0` | 138 | `S001` | 131 | 0.949275362319 | 6 |
| 6 | `S001` | `other` | 134 | `S001` | 63 | 0.470149253731 | 13 |
| 7 | `S003` | `2379-1` | 127 | `S001` | 121 | 0.952755905512 | 6 |
| 8 | `S003` | `other` | 112 | `S002` | 38 | 0.339285714286 | 13 |
| 9 | `S004` | `2379+0` | 74 | `S004` | 74 | 1.000000000000 | 1 |
| 10 | `S003` | `2380+0` | 64 | `S001` | 59 | 0.921875000000 | 4 |
| 11 | `S003` | `2379-2` | 45 | `S001` | 43 | 0.955555555556 | 3 |
| 12 | `S005` | `508+0` | 44 | `S001` | 42 | 0.954545454545 | 3 |
| 13 | `S001` | `341-3` | 44 | `S003` | 40 | 0.909090909091 | 5 |
| 14 | `S003` | `2379-3` | 43 | `S001` | 42 | 0.976744186047 | 2 |
| 15 | `S004` | `other` | 41 | `S004` | 20 | 0.487804878049 | 7 |
| 16 | `S003` | `2380+1` | 29 | `S001` | 26 | 0.896551724138 | 3 |
| 17 | `S004` | `2380+0` | 24 | `S004` | 24 | 1.000000000000 | 1 |
| 18 | `S002` | `2379+0` | 18 | `S001` | 17 | 0.944444444444 | 2 |
| 19 | `S002` | `2379-1` | 13 | `S001` | 10 | 0.769230769231 | 4 |
| 20 | `S003` | `508+1` | 13 | `S004` | 13 | 1.000000000000 | 1 |
| 21 | `S004` | `2380+1` | 11 | `S004` | 10 | 0.909090909091 | 2 |
| 22 | `S006` | `508+2` | 11 | `S003` | 8 | 0.727272727273 | 2 |
| 23 | `S002` | `2380+0` | 10 | `S001` | 8 | 0.800000000000 | 3 |

## Top transition enrichment

| rank | from | delta | to | observed | expected | enrichment |
|---:|---|---|---|---:|---:|---:|
| 1 | `S001` | `170+0` | `S002` | 729 | 252.461393003467 | 2.887570219459 |
| 2 | `S002` | `other` | `S001` | 481 | 216.287425149701 | 2.223892580288 |
| 3 | `S002` | `168-1` | `S003` | 347 | 67.268830759534 | 5.158406888926 |
| 4 | `S001` | `168+1` | `S002` | 149 | 50.168610148125 | 2.969984609103 |
| 5 | `S003` | `2379+0` | `S001` | 131 | 49.580838323353 | 2.642149758454 |
| 6 | `S003` | `2379-1` | `S001` | 121 | 45.628742514970 | 2.651837270341 |
| 7 | `S004` | `2379+0` | `S004` | 74 | 3.918058619603 | 18.886904761905 |
| 8 | `S001` | `other` | `S001` | 63 | 48.143712574850 | 1.308582089552 |
| 9 | `S003` | `2380+0` | `S001` | 59 | 22.994011976048 | 2.565885416667 |
| 10 | `S001` | `other` | `S003` | 54 | 24.832020170186 | 2.174611635699 |
| 11 | `S003` | `2379-2` | `S001` | 43 | 16.167664670659 | 2.659629629630 |
| 12 | `S005` | `508+0` | `S001` | 42 | 15.808383233533 | 2.656818181818 |
| 13 | `S003` | `2379-3` | `S001` | 42 | 15.449101796407 | 2.718604651163 |
| 14 | `S001` | `341-3` | `S003` | 40 | 8.153797667822 | 4.905689548547 |
| 15 | `S003` | `other` | `S002` | 38 | 36.250866687677 | 1.048250799833 |
| 16 | `S002` | `other` | `S002` | 36 | 194.848408446265 | 0.184759014903 |
| 17 | `S002` | `other` | `S003` | 32 | 111.558777182477 | 0.286844305829 |
| 18 | `S003` | `other` | `S001` | 31 | 40.239520958084 | 0.770386904762 |
| 19 | `S003` | `2380+1` | `S001` | 26 | 10.419161676647 | 2.495402298851 |
| 20 | `S004` | `2380+0` | `S004` | 24 | 1.270721714466 | 18.886904761905 |
| 21 | `S002` | `other` | `S005` | 24 | 9.865742199811 | 2.432660362893 |
| 22 | `S003` | `other` | `S005` | 22 | 1.835486920895 | 11.985920329670 |
| 23 | `S004` | `other` | `S004` | 20 | 2.170816262212 | 9.213124274100 |
| 24 | `S001` | `170+0` | `S003` | 20 | 144.544595020485 | 0.138365602651 |
| 25 | `S002` | `2379+0` | `S001` | 17 | 6.467065868263 | 2.628703703704 |
| 26 | `S003` | `508+1` | `S004` | 13 | 0.688307595336 | 18.886904761905 |
| 27 | `S003` | `other` | `S003` | 12 | 20.755121336275 | 0.578170553936 |
| 28 | `S004` | `2380+1` | `S004` | 10 | 0.582414119130 | 17.169913419913 |
| 29 | `S002` | `2379-1` | `S001` | 10 | 4.670658682635 | 2.141025641026 |

## Output files

```text
cluster_v06_null_summary.csv
cluster_v06_group_determinism.csv
cluster_v06_transition_enrichment.csv
cluster_v06_state_coverage.csv
cluster_v06_manifest.json
cluster_v06_report.md
```

## Interpretation guardrail

Wenn die beobachteten Determinismusmetriken weit außerhalb der Shuffle-Null liegen,
ist die Spacer-State-Folge intern nicht als einfache zufällige Anordnung der
beobachteten Zustände erklärbar. Ursprungsaussagen bleiben trotzdem externen
Kontrollen vorbehalten: RepeatMasker, Segmental-Duplication-Kontext, GC-Shuffles,
Cross-Chromosom-Scans und Build-/Populationsreplikation.
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v06_real\cluster_v06_null_summary.csv


metric         : transitions
observed       : 3173.000000000000
null_mean      :
null_sd        :
z              :
p_empirical_ge :
iterations     : 1000

metric         : groups
observed       : 178.000000000000
null_mean      :
null_sd        :
z              :
p_empirical_ge :
iterations     : 1000

metric         : weighted_top1_accuracy
observed       : 0.863220926568
null_mean      : 0.400930980145
null_sd        : 0.004065263392
z              : 113.717095759684
p_empirical_ge : 0.000999000999
iterations     : 1000

metric         : mean_group_top1_accuracy
observed       : 0.922785250163
null_mean      : 0.800243225363
null_sd        : 0.007892642919
z              : 15.526107800818
p_empirical_ge : 0.000999000999
iterations     : 1000

metric         : mean_group_entropy_bits
observed       : 0.236103541660
null_mean      : 0.572824050236
null_sd        : 0.017506115034
z              : -19.234450814760
p_empirical_ge : 1.000000000000
iterations     : 1000

metric         : top_transition_count
observed       : 729.000000000000
null_mean      : 281.126000000000
null_sd        : 10.583200083151
z              : 42.319335974102
p_empirical_ge : 0.000999000999
iterations     : 1000

metric         : top_transition_rate
observed       : 0.229751024267
null_mean      : 0.088599432714
null_sd        : 0.003335392399
z              : 42.319335974102
p_empirical_ge : 0.000999000999
iterations     : 1000

metric         : mutual_information_to_from_delta_bits
observed       : 1.700763070072
null_mean      : 0.298050521390
null_sd        : 0.011043853336
z              : 127.012964222917
p_empirical_ge : 0.000999000999
iterations     : 1000

metric         : max_same_state_run_length
observed       : 140.000000000000
null_mean      : 7.823000000000
null_sd        : 1.206511914570
z              : 109.553000184900
p_empirical_ge : 0.000999000999
iterations     : 1000

metric         : top_state_ngram_n3_count
observed       : 466.000000000000
null_mean      : 150.318000000000
null_sd        : 8.942979145676
z              : 35.299422581414
p_empirical_ge : 0.000999000999
iterations     : 1000



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v06_real\cluster_v06_group_determinism.csv |
>>   Select-Object -First 40


rank            : 1
from_state_id   : S001
delta_family    : 170+0
n               : 780
top_to_state_id : S002
top_count       : 729
top_rate        : 0.934615384615
entropy_bits    : 0.580952986754
alternatives    : 23
to_distribution : S002:729|S003:20|S011:3|S017:3|S031:2|S032:2|S014:2|S001:2|S035:2|S027:2|S046:1|S049:1

rank            : 2
from_state_id   : S002
delta_family    : other
n               : 602
top_to_state_id : S001
top_count       : 481
top_rate        : 0.799003322259
entropy_bits    : 1.327764613027
alternatives    : 25
to_distribution : S001:481|S002:36|S003:32|S005:24|S012:3|S008:3|S024:2|S013:2|S009:2|S018:2|S025:1|S047:1

rank            : 3
from_state_id   : S002
delta_family    : 168-1
n               : 363
top_to_state_id : S003
top_count       : 347
top_rate        : 0.955922865014
entropy_bits    : 0.369756468571
alternatives    : 9
to_distribution : S003:347|S002:7|S019:3|S022:1|S026:1|S029:1|S041:1|S001:1|S065:1

rank            : 4
from_state_id   : S001
delta_family    : 168+1
n               : 155
top_to_state_id : S002
top_count       : 149
top_rate        : 0.961290322581
entropy_bits    : 0.323504343669
alternatives    : 6
to_distribution : S002:149|S038:2|S022:1|S017:1|S003:1|S068:1

rank            : 5
from_state_id   : S003
delta_family    : 2379+0
n               : 138
top_to_state_id : S001
top_count       : 131
top_rate        : 0.949275362319
entropy_bits    : 0.402883781406
alternatives    : 6
to_distribution : S001:131|S002:2|S009:2|S013:1|S052:1|S021:1

rank            : 6
from_state_id   : S001
delta_family    : other
n               : 134
top_to_state_id : S001
top_count       : 63
top_rate        : 0.470149253731
entropy_bits    : 1.814618382443
alternatives    : 13
to_distribution : S001:63|S003:54|S002:5|S005:3|S045:1|S016:1|S023:1|S019:1|S008:1|S024:1|S063:1|S029:1

rank            : 7
from_state_id   : S003
delta_family    : 2379-1
n               : 127
top_to_state_id : S001
top_count       : 121
top_rate        : 0.952755905512
entropy_bits    : 0.380948846534
alternatives    : 6
to_distribution : S001:121|S009:2|S008:1|S013:1|S003:1|S018:1

rank            : 8
from_state_id   : S003
delta_family    : other
n               : 112
top_to_state_id : S002
top_count       : 38
top_rate        : 0.339285714286
entropy_bits    : 2.395494415348
alternatives    : 13
to_distribution : S002:38|S001:31|S005:22|S003:12|S004:1|S033:1|S009:1|S037:1|S020:1|S062:1|S014:1|S066:1

rank            : 9
from_state_id   : S004
delta_family    : 2379+0
n               : 74
top_to_state_id : S004
top_count       : 74
top_rate        : 1.000000000000
entropy_bits    : 0.000000000000
alternatives    : 1
to_distribution : S004:74

rank            : 10
from_state_id   : S003
delta_family    : 2380+0
n               : 64
top_to_state_id : S001
top_count       : 59
top_rate        : 0.921875000000
entropy_bits    : 0.514438438870
alternatives    : 4
to_distribution : S001:59|S028:2|S012:2|S021:1

rank            : 11
from_state_id   : S003
delta_family    : 2379-2
n               : 45
top_to_state_id : S001
top_count       : 43
top_rate        : 0.955555555556
entropy_bits    : 0.306755664059
alternatives    : 3
to_distribution : S001:43|S040:1|S013:1

rank            : 12
from_state_id   : S005
delta_family    : 508+0
n               : 44
top_to_state_id : S001
top_count       : 42
top_rate        : 0.954545454545
entropy_bits    : 0.312219533258
alternatives    : 3
to_distribution : S001:42|S018:1|S003:1

rank            : 13
from_state_id   : S001
delta_family    : 341-3
n               : 44
top_to_state_id : S003
top_count       : 40
top_rate        : 0.909090909091
entropy_bits    : 0.621315168740
alternatives    : 5
to_distribution : S003:40|S011:1|S037:1|S067:1|S029:1

rank            : 14
from_state_id   : S003
delta_family    : 2379-3
n               : 43
top_to_state_id : S001
top_count       : 42
top_rate        : 0.976744186047
entropy_bits    : 0.159350062686
alternatives    : 2
to_distribution : S001:42|S021:1

rank            : 15
from_state_id   : S004
delta_family    : other
n               : 41
top_to_state_id : S004
top_count       : 20
top_rate        : 0.487804878049
entropy_bits    : 2.075173287907
alternatives    : 7
to_distribution : S004:20|S006:9|S003:5|S015:4|S002:1|S022:1|S069:1

rank            : 16
from_state_id   : S003
delta_family    : 2380+1
n               : 29
top_to_state_id : S001
top_count       : 26
top_rate        : 0.896551724138
entropy_bits    : 0.574828144380
alternatives    : 3
to_distribution : S001:26|S002:2|S011:1

rank            : 17
from_state_id   : S004
delta_family    : 2380+0
n               : 24
top_to_state_id : S004
top_count       : 24
top_rate        : 1.000000000000
entropy_bits    : 0.000000000000
alternatives    : 1
to_distribution : S004:24

rank            : 18
from_state_id   : S002
delta_family    : 2379+0
n               : 18
top_to_state_id : S001
top_count       : 17
top_rate        : 0.944444444444
entropy_bits    : 0.309543429150
alternatives    : 2
to_distribution : S001:17|S002:1

rank            : 19
from_state_id   : S002
delta_family    : 2379-1
n               : 13
top_to_state_id : S001
top_count       : 10
top_rate        : 0.769230769231
entropy_bits    : 1.145110414382
alternatives    : 4
to_distribution : S001:10|S021:1|S012:1|S036:1

rank            : 20
from_state_id   : S003
delta_family    : 508+1
n               : 13
top_to_state_id : S004
top_count       : 13
top_rate        : 1.000000000000
entropy_bits    : 0.000000000000
alternatives    : 1
to_distribution : S004:13

rank            : 21
from_state_id   : S004
delta_family    : 2380+1
n               : 11
top_to_state_id : S004
top_count       : 10
top_rate        : 0.909090909091
entropy_bits    : 0.439496986922
alternatives    : 2
to_distribution : S004:10|S006:1

rank            : 22
from_state_id   : S006
delta_family    : 508+2
n               : 11
top_to_state_id : S003
top_count       : 8
top_rate        : 0.727272727273
entropy_bits    : 0.845350936622
alternatives    : 2
to_distribution : S003:8|S010:3

rank            : 23
from_state_id   : S002
delta_family    : 2380+0
n               : 10
top_to_state_id : S001
top_count       : 8
top_rate        : 0.800000000000
entropy_bits    : 0.921928094887
alternatives    : 3
to_distribution : S001:8|S048:1|S008:1



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v06_real\cluster_v06_transition_enrichment.csv |
>>   Select-Object -First 40


rank                    : 1
from_state_id           : S001
delta_family            : 170+0
to_state_id             : S002
observed                : 729
group_n                 : 780
to_state_global_n       : 1027
expected_independent    : 252.461393003467
enrichment              : 2.887570219459
observed_minus_expected : 476.538606996533

rank                    : 2
from_state_id           : S002
delta_family            : other
to_state_id             : S001
observed                : 481
group_n                 : 602
to_state_global_n       : 1140
expected_independent    : 216.287425149701
enrichment              : 2.223892580288
observed_minus_expected : 264.712574850299

rank                    : 3
from_state_id           : S002
delta_family            : 168-1
to_state_id             : S003
observed                : 347
group_n                 : 363
to_state_global_n       : 588
expected_independent    : 67.268830759534
enrichment              : 5.158406888926
observed_minus_expected : 279.731169240466

rank                    : 4
from_state_id           : S001
delta_family            : 168+1
to_state_id             : S002
observed                : 149
group_n                 : 155
to_state_global_n       : 1027
expected_independent    : 50.168610148125
enrichment              : 2.969984609103
observed_minus_expected : 98.831389851875

rank                    : 5
from_state_id           : S003
delta_family            : 2379+0
to_state_id             : S001
observed                : 131
group_n                 : 138
to_state_global_n       : 1140
expected_independent    : 49.580838323353
enrichment              : 2.642149758454
observed_minus_expected : 81.419161676647

rank                    : 6
from_state_id           : S003
delta_family            : 2379-1
to_state_id             : S001
observed                : 121
group_n                 : 127
to_state_global_n       : 1140
expected_independent    : 45.628742514970
enrichment              : 2.651837270341
observed_minus_expected : 75.371257485030

rank                    : 7
from_state_id           : S004
delta_family            : 2379+0
to_state_id             : S004
observed                : 74
group_n                 : 74
to_state_global_n       : 168
expected_independent    : 3.918058619603
enrichment              : 18.886904761905
observed_minus_expected : 70.081941380397

rank                    : 8
from_state_id           : S001
delta_family            : other
to_state_id             : S001
observed                : 63
group_n                 : 134
to_state_global_n       : 1140
expected_independent    : 48.143712574850
enrichment              : 1.308582089552
observed_minus_expected : 14.856287425150

rank                    : 9
from_state_id           : S003
delta_family            : 2380+0
to_state_id             : S001
observed                : 59
group_n                 : 64
to_state_global_n       : 1140
expected_independent    : 22.994011976048
enrichment              : 2.565885416667
observed_minus_expected : 36.005988023952

rank                    : 10
from_state_id           : S001
delta_family            : other
to_state_id             : S003
observed                : 54
group_n                 : 134
to_state_global_n       : 588
expected_independent    : 24.832020170186
enrichment              : 2.174611635699
observed_minus_expected : 29.167979829814

rank                    : 11
from_state_id           : S003
delta_family            : 2379-2
to_state_id             : S001
observed                : 43
group_n                 : 45
to_state_global_n       : 1140
expected_independent    : 16.167664670659
enrichment              : 2.659629629630
observed_minus_expected : 26.832335329341

rank                    : 12
from_state_id           : S005
delta_family            : 508+0
to_state_id             : S001
observed                : 42
group_n                 : 44
to_state_global_n       : 1140
expected_independent    : 15.808383233533
enrichment              : 2.656818181818
observed_minus_expected : 26.191616766467

rank                    : 13
from_state_id           : S003
delta_family            : 2379-3
to_state_id             : S001
observed                : 42
group_n                 : 43
to_state_global_n       : 1140
expected_independent    : 15.449101796407
enrichment              : 2.718604651163
observed_minus_expected : 26.550898203593

rank                    : 14
from_state_id           : S001
delta_family            : 341-3
to_state_id             : S003
observed                : 40
group_n                 : 44
to_state_global_n       : 588
expected_independent    : 8.153797667822
enrichment              : 4.905689548547
observed_minus_expected : 31.846202332178

rank                    : 15
from_state_id           : S003
delta_family            : other
to_state_id             : S002
observed                : 38
group_n                 : 112
to_state_global_n       : 1027
expected_independent    : 36.250866687677
enrichment              : 1.048250799833
observed_minus_expected : 1.749133312323

rank                    : 16
from_state_id           : S002
delta_family            : other
to_state_id             : S002
observed                : 36
group_n                 : 602
to_state_global_n       : 1027
expected_independent    : 194.848408446265
enrichment              : 0.184759014903
observed_minus_expected : -158.848408446265

rank                    : 17
from_state_id           : S002
delta_family            : other
to_state_id             : S003
observed                : 32
group_n                 : 602
to_state_global_n       : 588
expected_independent    : 111.558777182477
enrichment              : 0.286844305829
observed_minus_expected : -79.558777182477

rank                    : 18
from_state_id           : S003
delta_family            : other
to_state_id             : S001
observed                : 31
group_n                 : 112
to_state_global_n       : 1140
expected_independent    : 40.239520958084
enrichment              : 0.770386904762
observed_minus_expected : -9.239520958084

rank                    : 19
from_state_id           : S003
delta_family            : 2380+1
to_state_id             : S001
observed                : 26
group_n                 : 29
to_state_global_n       : 1140
expected_independent    : 10.419161676647
enrichment              : 2.495402298851
observed_minus_expected : 15.580838323353

rank                    : 20
from_state_id           : S004
delta_family            : 2380+0
to_state_id             : S004
observed                : 24
group_n                 : 24
to_state_global_n       : 168
expected_independent    : 1.270721714466
enrichment              : 18.886904761905
observed_minus_expected : 22.729278285534

rank                    : 21
from_state_id           : S002
delta_family            : other
to_state_id             : S005
observed                : 24
group_n                 : 602
to_state_global_n       : 52
expected_independent    : 9.865742199811
enrichment              : 2.432660362893
observed_minus_expected : 14.134257800189

rank                    : 22
from_state_id           : S003
delta_family            : other
to_state_id             : S005
observed                : 22
group_n                 : 112
to_state_global_n       : 52
expected_independent    : 1.835486920895
enrichment              : 11.985920329670
observed_minus_expected : 20.164513079105

rank                    : 23
from_state_id           : S004
delta_family            : other
to_state_id             : S004
observed                : 20
group_n                 : 41
to_state_global_n       : 168
expected_independent    : 2.170816262212
enrichment              : 9.213124274100
observed_minus_expected : 17.829183737788

rank                    : 24
from_state_id           : S001
delta_family            : 170+0
to_state_id             : S003
observed                : 20
group_n                 : 780
to_state_global_n       : 588
expected_independent    : 144.544595020485
enrichment              : 0.138365602651
observed_minus_expected : -124.544595020485

rank                    : 25
from_state_id           : S002
delta_family            : 2379+0
to_state_id             : S001
observed                : 17
group_n                 : 18
to_state_global_n       : 1140
expected_independent    : 6.467065868263
enrichment              : 2.628703703704
observed_minus_expected : 10.532934131737

rank                    : 26
from_state_id           : S003
delta_family            : 508+1
to_state_id             : S004
observed                : 13
group_n                 : 13
to_state_global_n       : 168
expected_independent    : 0.688307595336
enrichment              : 18.886904761905
observed_minus_expected : 12.311692404664

rank                    : 27
from_state_id           : S003
delta_family            : other
to_state_id             : S003
observed                : 12
group_n                 : 112
to_state_global_n       : 588
expected_independent    : 20.755121336275
enrichment              : 0.578170553936
observed_minus_expected : -8.755121336275

rank                    : 28
from_state_id           : S004
delta_family            : 2380+1
to_state_id             : S004
observed                : 10
group_n                 : 11
to_state_global_n       : 168
expected_independent    : 0.582414119130
enrichment              : 17.169913419913
observed_minus_expected : 9.417585880870

rank                    : 29
from_state_id           : S002
delta_family            : 2379-1
to_state_id             : S001
observed                : 10
group_n                 : 13
to_state_global_n       : 1140
expected_independent    : 4.670658682635
enrichment              : 2.141025641026
observed_minus_expected : 5.329341317365



PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v06_real\* `
>>   -DestinationPath .\Decode_chr17_v06_real_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun>


PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_evidence_gate_v0_7.ps1 `
>>   -DeterminismDir .\Decode_chr17_v06_real `
>>   -SequenceDir .\Decode_chr17_v04_real `
>>   -OutDir .\Decode_chr17_v07_real
KALYX evidence gate v0.7 complete: Decode_chr17_v07_real
  gates_passed=6 gates_open=0 gates_missing=0
  report=Decode_chr17_v07_real\cluster_v07_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v07_real\cluster_v07_report.md
# KALYX Evidence Gate v0.7

## Boundary

Dieses Artefakt ist eine Claim-Gate-Schicht. Es beweist keinen Ursprung.
Es sagt, welche Aussage durch die bisherigen Kontrollen getragen wird und welche Kontrollen noch fehlen.

## Summary

- gates passed: `6`
- gates open: `0`
- gates missing: `0`
- allowed statement: **The spacer-state automaton is strongly ordered relative to the internal state-frequency shuffle null.**
- prohibited statement: **Artificial origin is proven.**

## Evidence gates

| gate | status | observed | null_mean | z | p |
|---|---|---:|---:|---:|---:|
| `weighted_top1_accuracy` | `pass` | 0.863220926568 | 0.400930980145 | 113.717095759684 | 0.000999000999 |
| `mutual_information_to_from_delta_bits` | `pass` | 1.700763070072 | 0.298050521390 | 127.012964222917 | 0.000999000999 |
| `top_transition_count` | `pass` | 729.000000000000 | 281.126000000000 | 42.319335974102 | 0.000999000999 |
| `max_same_state_run_length` | `pass` | 140.000000000000 | 7.823000000000 | 109.553000184900 | 0.000999000999 |
| `top_state_ngram_n3_count` | `pass` | 466.000000000000 | 150.318000000000 | 35.299422581414 | 0.000999000999 |
| `mean_group_entropy_bits` | `pass` | 0.236103541660 | 0.572824050236 | -19.234450814760 | 1.000000000000 |

## Control requirements

| control | status | required_for |
|---|---|---|
| `internal_state_frequency_shuffle` | `done` | ordered automaton claim |
| `window_stratified_shuffle` | `next` | exclude window composition artefact |
| `repeatmasker_overlap` | `missing` | repeat explanation assessment |
| `segmental_duplication_overlap` | `missing` | duplication explanation assessment |
| `gc_matched_local_shuffle` | `missing` | sequence composition null |
| `cross_chromosome_scan` | `missing` | genome-wide specificity |
| `cross_build_or_population_replication` | `missing` | reference-build artefact exclusion |

## Claim ladder

1. **Now supported:** The spacer-state automaton is strongly ordered relative to the internal shuffle null.
2. **Next supported if v0.7+ controls pass:** The observed order is not explained by tested local sequence/composition/repeat nulls.
3. **Still not proven by this layer:** artificial origin.

## Output files

```text
cluster_v07_evidence_gates.csv
cluster_v07_control_requirements.csv
cluster_v07_top_transition_enrichment.csv
cluster_v07_repeat_overlap.csv                optional
cluster_v07_repeat_overlap_summary.csv        optional
cluster_v07_manifest.json
cluster_v07_report.md
```
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v07_real\cluster_v07_evidence_gates.csv


gate        : weighted_top1_accuracy
status      : pass
observed    : 0.863220926568
null_mean   : 0.400930980145
z           : 113.717095759684
p_empirical : 0.000999000999
explanation : Transition groups predict next state better than state-frequency shuffle.

gate        : mutual_information_to_from_delta_bits
status      : pass
observed    : 1.700763070072
null_mean   : 0.298050521390
z           : 127.012964222917
p_empirical : 0.000999000999
explanation : Next state carries information about from-state/delta group.

gate        : top_transition_count
status      : pass
observed    : 729.000000000000
null_mean   : 281.126000000000
z           : 42.319335974102
p_empirical : 0.000999000999
explanation : Dominant transition exceeds shuffled-order baseline.

gate        : max_same_state_run_length
status      : pass
observed    : 140.000000000000
null_mean   : 7.823000000000
z           : 109.553000184900
p_empirical : 0.000999000999
explanation : Observed same-state runs exceed shuffled baseline.

gate        : top_state_ngram_n3_count
status      : pass
observed    : 466.000000000000
null_mean   : 150.318000000000
z           : 35.299422581414
p_empirical : 0.000999000999
explanation : Observed state tri-gram exceeds shuffled baseline.

gate        : mean_group_entropy_bits
status      : pass
observed    : 0.236103541660
null_mean   : 0.572824050236
z           : -19.234450814760
p_empirical : 1.000000000000
explanation : Transition groups are lower-entropy than shuffled baseline.



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v07_real\cluster_v07_control_requirements.csv

control                               status  evidence          required_for
-------                               ------  --------          ------------
internal_state_frequency_shuffle      done    v0.6 null_summary ordered automaton claim
window_stratified_shuffle             next                      exclude window composition artefact
repeatmasker_overlap                  missing                   repeat explanation assessment
segmental_duplication_overlap         missing                   duplication explanation assessment
gc_matched_local_shuffle              missing                   sequence composition null
cross_chromosome_scan                 missing                   genome-wide specificity
cross_build_or_population_replication missing                   reference-build artefact exclusion


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v07_real\cluster_v07_top_transition_enrichment.csv |
>>   Select-Object -First 40


rank                    : 1
from_state_id           : S001
delta_family            : 170+0
to_state_id             : S002
observed                : 729
expected_independent    : 252.461393003467
enrichment              : 2.887570219459
observed_minus_expected : 476.538606996533

rank                    : 2
from_state_id           : S002
delta_family            : other
to_state_id             : S001
observed                : 481
expected_independent    : 216.287425149701
enrichment              : 2.223892580288
observed_minus_expected : 264.712574850299

rank                    : 3
from_state_id           : S002
delta_family            : 168-1
to_state_id             : S003
observed                : 347
expected_independent    : 67.268830759534
enrichment              : 5.158406888926
observed_minus_expected : 279.731169240466

rank                    : 4
from_state_id           : S001
delta_family            : 168+1
to_state_id             : S002
observed                : 149
expected_independent    : 50.168610148125
enrichment              : 2.969984609103
observed_minus_expected : 98.831389851875

rank                    : 5
from_state_id           : S003
delta_family            : 2379+0
to_state_id             : S001
observed                : 131
expected_independent    : 49.580838323353
enrichment              : 2.642149758454
observed_minus_expected : 81.419161676647

rank                    : 6
from_state_id           : S003
delta_family            : 2379-1
to_state_id             : S001
observed                : 121
expected_independent    : 45.628742514970
enrichment              : 2.651837270341
observed_minus_expected : 75.371257485030

rank                    : 7
from_state_id           : S004
delta_family            : 2379+0
to_state_id             : S004
observed                : 74
expected_independent    : 3.918058619603
enrichment              : 18.886904761905
observed_minus_expected : 70.081941380397

rank                    : 8
from_state_id           : S001
delta_family            : other
to_state_id             : S001
observed                : 63
expected_independent    : 48.143712574850
enrichment              : 1.308582089552
observed_minus_expected : 14.856287425150

rank                    : 9
from_state_id           : S003
delta_family            : 2380+0
to_state_id             : S001
observed                : 59
expected_independent    : 22.994011976048
enrichment              : 2.565885416667
observed_minus_expected : 36.005988023952

rank                    : 10
from_state_id           : S001
delta_family            : other
to_state_id             : S003
observed                : 54
expected_independent    : 24.832020170186
enrichment              : 2.174611635699
observed_minus_expected : 29.167979829814

rank                    : 11
from_state_id           : S003
delta_family            : 2379-2
to_state_id             : S001
observed                : 43
expected_independent    : 16.167664670659
enrichment              : 2.659629629630
observed_minus_expected : 26.832335329341

rank                    : 12
from_state_id           : S005
delta_family            : 508+0
to_state_id             : S001
observed                : 42
expected_independent    : 15.808383233533
enrichment              : 2.656818181818
observed_minus_expected : 26.191616766467

rank                    : 13
from_state_id           : S003
delta_family            : 2379-3
to_state_id             : S001
observed                : 42
expected_independent    : 15.449101796407
enrichment              : 2.718604651163
observed_minus_expected : 26.550898203593

rank                    : 14
from_state_id           : S001
delta_family            : 341-3
to_state_id             : S003
observed                : 40
expected_independent    : 8.153797667822
enrichment              : 4.905689548547
observed_minus_expected : 31.846202332178

rank                    : 15
from_state_id           : S003
delta_family            : other
to_state_id             : S002
observed                : 38
expected_independent    : 36.250866687677
enrichment              : 1.048250799833
observed_minus_expected : 1.749133312323

rank                    : 16
from_state_id           : S002
delta_family            : other
to_state_id             : S002
observed                : 36
expected_independent    : 194.848408446265
enrichment              : 0.184759014903
observed_minus_expected : -158.848408446265

rank                    : 17
from_state_id           : S002
delta_family            : other
to_state_id             : S003
observed                : 32
expected_independent    : 111.558777182477
enrichment              : 0.286844305829
observed_minus_expected : -79.558777182477

rank                    : 18
from_state_id           : S003
delta_family            : other
to_state_id             : S001
observed                : 31
expected_independent    : 40.239520958084
enrichment              : 0.770386904762
observed_minus_expected : -9.239520958084

rank                    : 19
from_state_id           : S003
delta_family            : 2380+1
to_state_id             : S001
observed                : 26
expected_independent    : 10.419161676647
enrichment              : 2.495402298851
observed_minus_expected : 15.580838323353

rank                    : 20
from_state_id           : S004
delta_family            : 2380+0
to_state_id             : S004
observed                : 24
expected_independent    : 1.270721714466
enrichment              : 18.886904761905
observed_minus_expected : 22.729278285534

rank                    : 21
from_state_id           : S002
delta_family            : other
to_state_id             : S005
observed                : 24
expected_independent    : 9.865742199811
enrichment              : 2.432660362893
observed_minus_expected : 14.134257800189

rank                    : 22
from_state_id           : S003
delta_family            : other
to_state_id             : S005
observed                : 22
expected_independent    : 1.835486920895
enrichment              : 11.985920329670
observed_minus_expected : 20.164513079105

rank                    : 23
from_state_id           : S004
delta_family            : other
to_state_id             : S004
observed                : 20
expected_independent    : 2.170816262212
enrichment              : 9.213124274100
observed_minus_expected : 17.829183737788

rank                    : 24
from_state_id           : S001
delta_family            : 170+0
to_state_id             : S003
observed                : 20
expected_independent    : 144.544595020485
enrichment              : 0.138365602651
observed_minus_expected : -124.544595020485

rank                    : 25
from_state_id           : S002
delta_family            : 2379+0
to_state_id             : S001
observed                : 17
expected_independent    : 6.467065868263
enrichment              : 2.628703703704
observed_minus_expected : 10.532934131737

rank                    : 26
from_state_id           : S003
delta_family            : 508+1
to_state_id             : S004
observed                : 13
expected_independent    : 0.688307595336
enrichment              : 18.886904761905
observed_minus_expected : 12.311692404664

rank                    : 27
from_state_id           : S003
delta_family            : other
to_state_id             : S003
observed                : 12
expected_independent    : 20.755121336275
enrichment              : 0.578170553936
observed_minus_expected : -8.755121336275

rank                    : 28
from_state_id           : S004
delta_family            : 2380+1
to_state_id             : S004
observed                : 10
expected_independent    : 0.582414119130
enrichment              : 17.169913419913
observed_minus_expected : 9.417585880870

rank                    : 29
from_state_id           : S002
delta_family            : 2379-1
to_state_id             : S001
observed                : 10
expected_independent    : 4.670658682635
enrichment              : 2.141025641026
observed_minus_expected : 5.329341317365



PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v07_real\* `
>>   -DestinationPath .\Decode_chr17_v07_real_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun>

PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_natural_model_breaker_v0_8.ps1 `
>>   -SequenceDir .\Decode_chr17_v04_real `
>>   -StateDir .\Decode_chr17_v05_real `
>>   -DeterminismDir .\Decode_chr17_v06_real `
>>   -EvidenceDir .\Decode_chr17_v07_real `
>>   -Fasta $FA `
>>   -ChromDir .\Genome `
>>   -RepeatMaskerBed $RM `
>>   -SegmentalDupBed $SD `
>>   -OutDir .\Decode_chr17_v08_preview_with_controls_abs `
>>   -Iterations 1000
KALYX Natural Model Breaker v0.8 complete: Decode_chr17_v08_preview_with_controls_abs
  final_status=not_released
  final_statement=Der finale Satz ist noch nicht freigegeben; mindestens eine externe Kontrollklasse fehlt, ist nicht gebrochen oder ist inkonklusiv.
  report=Decode_chr17_v08_preview_with_controls_abs\cluster_v08_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v08_preview_with_controls_abs\cluster_v08_report.md
# KALYX Natural Model Breaker v0.8

## Boundary

Dieses Artefakt ist die letzte Kontrollschicht für den bisherigen Lauf. Es beweist keinen künstlichen Ursprung. Es prüft, ob die in diesem Lauf tatsächlich getesteten natürlichen Erklärungsmodelle den Befund noch tragen.

## Final status

- final_status: `not_released`
- final_statement: **Der finale Satz ist noch nicht freigegeben; mindestens eine externe Kontrollklasse fehlt, ist nicht gebrochen oder ist inkonklusiv.**
- observed template-3 / A-Spacer-B blocks: `3174`

## Gates

| control | status | required | note |
|---|---|---|---|
| `internal_evidence_gate_v07` | `pass` | `yes` | {"control": "internal_evidence_gate_v07", "status": "pass", "gates": 6} |
| `window_stratified_shuffle` | `fail` | `yes` | {"control": "window_stratified_shuffle", "status": "fail", "passed_core_metrics": 3, "required_core_metrics": 4} |
| `gc_matched_local_shuffle` | `pass` | `yes` | {"control": "gc_matched_local_shuffle", "status": "pass", "observed_template3_clusters": 3174, "null_mean": 0.0, "null_sd": 0.0, "z": Infinity, "p_empirical_ge": 0.000999000999000999, "iterations": 1000, "gc": 0.4029725257238773, "length":  |
| `repeatmasker_overlap` | `natural_model_not_broken` | `yes` | {"control": "repeatmasker_overlap", "status": "natural_model_not_broken", "overlap_blocks_fraction": 1.0, "overlap_bp_fraction": 0.9999932965986942} |
| `segmental_duplication_overlap` | `pass` | `yes` | {"control": "segmental_duplication_overlap", "status": "pass", "overlap_blocks_fraction": 0.0, "overlap_bp_fraction": 0.0} |
| `cross_chromosome_scan` | `natural_model_not_broken` | `yes` | {"control": "cross_chromosome_scan", "status": "natural_model_not_broken", "observed_top_full47_total_chr17_blocks": 3067, "top_n_full47": 20, "max_non17_full47_hits": 114, "total_non17_full47_hits": 295, "threshold_1pct_observed": 30, "fas |

## Window-stratified shuffle

| metric | observed | null_mean | z | p | status |
|---|---:|---:|---:|---:|---|
| `weighted_top1_accuracy` | 0.863220926568 | 0.446911755436 | 92.870780152444 | 0.000999000999 | `pass` |
| `top_transition_count` | 729.000000000000 | 137.604000000000 | 91.980853028173 | 0.000999000999 | `pass` |
| `mutual_information_to_from_delta_bits` | 1.700763070072 | 0.554891051258 | 92.679264371216 | 0.000999000999 | `pass` |
| `max_same_state_run_length` | 140.000000000000 | 74.441000000000 | 4.449240550842 | 0.000999000999 | `fail` |

## GC-matched local shuffle

- status: `pass`
- observed template-3 blocks: `3174`
- null mean: `0.0`
- z: `inf`
- p_empirical_ge: `0.000999000999000999`
- source: `fasta_local_region`

## Interpretation

Mindestens ein erforderliches externes Gate fehlt, bleibt inkonklusiv oder trägt ein natürliches Modell weiter. Der finale Satz bleibt deshalb gesperrt, bis die fehlenden Daten nachgereicht und bestanden sind.

## Output files

```text
cluster_v08_control_gates.csv
cluster_v08_window_shuffle_summary.csv
cluster_v08_gc_shuffle_iterations.csv
cluster_v08_repeatmasker_overlap.csv        optional
cluster_v08_repeatmasker_summary.csv        optional
cluster_v08_segmental_dup_overlap.csv       optional
cluster_v08_segmental_dup_summary.csv       optional
cluster_v08_cross_chromosome_full47.csv
cluster_v08_manifest.json
cluster_v08_report.md
```
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v08_preview_with_controls_abs\cluster_v08_control_gates.csv


control                  : internal_evidence_gate_v07
status                   : pass
required_for_final_claim : yes
observed                 :
null_or_reference        :
z                        :
p                        :
note                     : {"control": "internal_evidence_gate_v07", "status": "pass", "gates": 6}

control                  : window_stratified_shuffle
status                   : fail
required_for_final_claim : yes
observed                 :
null_or_reference        :
z                        :
p                        :
note                     : {"control": "window_stratified_shuffle", "status": "fail", "passed_core_metrics": 3,
                           "required_core_metrics": 4}

control                  : gc_matched_local_shuffle
status                   : pass
required_for_final_claim : yes
observed                 : 3174
null_or_reference        : mean=0.0
z                        : inf
p                        : 0.000999000999000999
note                     : {"control": "gc_matched_local_shuffle", "status": "pass", "observed_template3_clusters":
                           3174, "null_mean": 0.0, "null_sd": 0.0, "z": Infinity, "p_empirical_ge":
                           0.000999000999000999, "iterations": 1000, "gc": 0.4029725257238773, "length": 3965382,
                           "source": "fasta_local_region"}

control                  : repeatmasker_overlap
status                   : natural_model_not_broken
required_for_final_claim : yes
observed                 :
null_or_reference        :
z                        :
p                        :
note                     : {"control": "repeatmasker_overlap", "status": "natural_model_not_broken",
                           "overlap_blocks_fraction": 1.0, "overlap_bp_fraction": 0.9999932965986942}

control                  : segmental_duplication_overlap
status                   : pass
required_for_final_claim : yes
observed                 :
null_or_reference        :
z                        :
p                        :
note                     : {"control": "segmental_duplication_overlap", "status": "pass", "overlap_blocks_fraction":
                           0.0, "overlap_bp_fraction": 0.0}

control                  : cross_chromosome_scan
status                   : natural_model_not_broken
required_for_final_claim : yes
observed                 :
null_or_reference        :
z                        :
p                        :
note                     : {"control": "cross_chromosome_scan", "status": "natural_model_not_broken",
                           "observed_top_full47_total_chr17_blocks": 3067, "top_n_full47": 20,
                           "max_non17_full47_hits": 114, "total_non17_full47_hits": 295, "threshold_1pct_observed":
                           30, "fastas_scanned": 24}



PS H:\kdna_opencl_with_inspector_krun> Get-ChildItem .\Decode_chr17_v08_preview_with_controls_abs |
>>   Sort-Object Name


    Verzeichnis: H:\kdna_opencl_with_inspector_krun\Decode_chr17_v08_preview_with_controls_abs


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        02.06.2026     13:06           1489 cluster_v08_control_gates.csv
-a----        02.06.2026     13:06          29596 cluster_v08_cross_chromosome_full47.csv
-a----        02.06.2026     13:04          31944 cluster_v08_gc_shuffle_iterations.csv
-a----        02.06.2026     13:06           3425 cluster_v08_manifest.json
-a----        02.06.2026     13:04         226799 cluster_v08_repeatmasker_overlap.csv
-a----        02.06.2026     13:04            464 cluster_v08_repeatmasker_summary.csv
-a----        02.06.2026     13:06           3354 cluster_v08_report.md
-a----        02.06.2026     13:04         144275 cluster_v08_segmental_dup_overlap.csv
-a----        02.06.2026     13:04            378 cluster_v08_segmental_dup_summary.csv
-a----        02.06.2026     12:46            744 cluster_v08_window_shuffle_summary.csv


PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v08_preview_with_controls_abs\cluster_v08_repeatmasker_summary.csv


control                 : repeatmasker_overlap
bed                     : H:\kdna_opencl_with_inspector_krun\Controls_UCSC_hg38\repeatmasker_chr17.bed
blocks                  : 3174
overlap_blocks          : 3174
overlap_blocks_fraction : 1.000000000000
total_block_bp          : 149178
overlap_bp              : 149177
overlap_bp_fraction     : 0.999993296599
top_annotations         : ALR/Alpha|Satellite|centr:3174
status                  : natural_model_not_broken
interpretation          : Most blocks overlap this annotation class; this natural model remains plausible and needs
                          stratified follow-up.



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v08_preview_with_controls_abs\cluster_v08_repeatmasker_overlap.csv |
>>   Select-Object -First 30


cluster_id       : 37
start0           : 22747231
end0             : 22747278
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 59
start0           : 22751986
end0             : 22752033
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 70
start0           : 22754365
end0             : 22754412
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 79
start0           : 22756744
end0             : 22756791
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 88
start0           : 22759122
end0             : 22759169
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 99
start0           : 22761501
end0             : 22761548
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 115
start0           : 22815062
end0             : 22815109
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 119
start0           : 22816077
end0             : 22816124
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 146
start0           : 22822029
end0             : 22822076
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 156
start0           : 22824408
end0             : 22824455
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 184
start0           : 22831545
end0             : 22831592
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 193
start0           : 22833925
end0             : 22833972
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 201
start0           : 22836304
end0             : 22836351
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 211
start0           : 22838683
end0             : 22838730
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 219
start0           : 22841062
end0             : 22841109
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 229
start0           : 22843442
end0             : 22843489
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 239
start0           : 22845820
end0             : 22845867
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 248
start0           : 22848199
end0             : 22848246
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 258
start0           : 22850578
end0             : 22850625
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 267
start0           : 22852957
end0             : 22853004
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 277
start0           : 22855336
end0             : 22855383
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 287
start0           : 22857714
end0             : 22857761
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 297
start0           : 22860093
end0             : 22860140
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 307
start0           : 22862472
end0             : 22862519
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 317
start0           : 22864851
end0             : 22864898
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 328
start0           : 22867230
end0             : 22867277
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 338
start0           : 22869609
end0             : 22869656
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 358
start0           : 22874367
end0             : 22874414
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 367
start0           : 22876746
end0             : 22876793
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr

cluster_id       : 377
start0           : 22879125
end0             : 22879172
span             : 47
overlap_bp       : 47
overlap_fraction : 1.000000000000
annotation_names : ALR/Alpha|Satellite|centr



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v08_preview_with_controls_abs\cluster_v08_repeatmasker_overlap.csv |
>>   Group-Object repClass |
>>   Sort-Object Count -Descending |
>>   Select-Object -First 20

Count Name                      Group
----- ----                      -----
 3174                           {@{cluster_id=37; start0=22747231; end0=22747278; span=47; overlap_bp=47; overlap_fr...


PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path `
>>     .\Decode_chr17_v01_real, `
>>     .\Decode_chr17_v02_real, `
>>     .\Decode_chr17_v03_real, `
>>     .\Decode_chr17_v04_real, `
>>     .\Decode_chr17_v05_real, `
>>     .\Decode_chr17_v06_real, `
>>     .\Decode_chr17_v07_real, `
>>     .\Decode_chr17_v08_preview_with_controls_abs, `
>>     .\Controls_UCSC_hg38 `
>>   -DestinationPath .\KALYX_chr17_full_run_v01_to_v08_with_controls.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path `
>>     .\Decode_chr17_v01_real, `
>>     .\Decode_chr17_v02_real, `
>>     .\Decode_chr17_v03_real, `
>>     .\Decode_chr17_v04_real, `
>>     .\Decode_chr17_v05_real, `
>>     .\Decode_chr17_v06_real, `
>>     .\Decode_chr17_v07_real, `
>>     .\Decode_chr17_v08_preview_with_controls_abs, `
>>     .\Controls_UCSC_hg38, `
>>     .\scripts\kalyx_signature_decode_v0_1.ps1, `
>>     .\scripts\kalyx_signature_cluster_v0_2.ps1, `
>>     .\scripts\kalyx_signature_block_v0_3.ps1, `
>>     .\scripts\kalyx_signature_sequence_v0_4.ps1, `
>>     .\scripts\kalyx_spacer_state_transition_v0_5.ps1, `
>>     .\scripts\kalyx_spacer_state_determinism_v0_6.ps1, `
>>     .\scripts\kalyx_evidence_gate_v0_7.ps1, `
>>     .\scripts\kalyx_natural_model_breaker_v0_8.ps1, `
>>     .\scripts\kalyx_build_ucsc_control_beds_v0_8_1.ps1, `
>>     .\python\kalyx_signature_decoder.py, `
>>     .\python\kalyx_signature_cluster_v0_2.py, `
>>     .\python\kalyx_signature_block_v0_3.py, `
>>     .\python\kalyx_signature_sequence_v0_4.py, `
>>     .\python\kalyx_spacer_state_transition_v0_5.py, `
>>     .\python\kalyx_spacer_state_determinism_v0_6.py, `
>>     .\python\kalyx_evidence_gate_v0_7.py, `
>>     .\python\kalyx_natural_model_breaker_v0_8.py, `
>>     .\python\kalyx_build_ucsc_control_beds_v0_8_1.py, `
>>     .\docs `
>>   -DestinationPath .\KALYX_chr17_full_run_v01_to_v08_with_code_and_controls.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun> Get-Item .\KALYX_chr17_full_run_v01_to_v08_with_code_and_controls.zip |
>>   Select-Object FullName,Length,LastWriteTime

FullName                                                                                         Length LastWriteTime
--------                                                                                         ------ -------------
H:\kdna_opencl_with_inspector_krun\KALYX_chr17_full_run_v01_to_v08_with_code_and_controls.zip 170355082 02.06.2026 1...


PS H:\kdna_opencl_with_inspector_krun> Get-FileHash .\KALYX_chr17_full_run_v01_to_v08_with_code_and_controls.zip -Algorithm SHA256

Algorithm       Hash                                                                   Path
---------       ----                                                                   ----
SHA256          5449EACBEAAFF003C91DB056E7D8B691289DBB2F3AE9EE057FA71058872FC593       H:\kdna_opencl_with_inspector...


PS H:\kdna_opencl_with_inspector_krun>



-----------------------------------------------



PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_repeat_family_automaton_v0_9.ps1 `
>>   -SequenceDir .\Decode_chr17_v04_real `
>>   -RepeatMaskerBed .\Controls_UCSC_hg38\repeatmasker_chr17.bed `
>>   -Fasta .\chr17.fa `
>>   -OutDir .\Decode_chr17_v09_final `
>>   -Iterations 1000 `
>>   -ScanFamily
KALYX repeat-family conditioned automaton test complete: Decode_chr17_v09_final
  blocks=3174 dominant_group='ALR/Alpha|Satellite|centr' dominant_blocks=3174
  claim_status=repeat_family_alone_not_sufficient
  report=Decode_chr17_v09_final\cluster_v09_report.md
PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v09_final\cluster_v09_report.md
# KALYX Repeat-Family Conditioned Automaton Test v0.9

## Boundary

Dieses Artefakt beweist keinen natürlichen oder künstlichen Ursprung. Es prüft
eine präzisere Frage als v0.8:

> Erklärt die Zugehörigkeit zur dominanten RepeatMasker-Familie allein den
> beobachteten A/Spacer/B-Spacer-State-Automaten?

## Summary

- observed blocks: `3174`
- repeat annotations: `170331`
- dominant repeat group: `ALR/Alpha|Satellite|centr`
- dominant group blocks: `3174`
- dominant group fraction: `1.000000000000`
- family skeleton scan enabled: `True`
- family skeleton A/Spacer/B blocks: `3173`
- shuffle iterations: `1000`
- claim_status: `repeat_family_alone_not_sufficient`

## Repeat group summary

| repeat_key | blocks | block_fraction | unique_spacers | top_spacer |
|---|---:|---:|---:|---|
| `ALR/Alpha|Satellite|centr` | 3174 | 1.000000000000 | 92 | `CCTTCTTCGTGATGTC` |

## Dominant-family transitions

| from_spacer | delta_family | to_spacer | count | conditional_rate |
|---|---|---|---:|---:|
| `CCTTCTTCGTGATGTC` | `170+0` | `CCCTCTTCGTGATGTT` | 729 | 31.695652173913 |
| `CCCTCTTCGTGATGTT` | `other` | `CCTTCTTCGTGATGTC` | 481 | 19.240000000000 |
| `CCCTCTTCGTGATGTT` | `168-1` | `CCTTCTTCGTGATGTT` | 347 | 38.555555555556 |
| `CCTTCTTCGTGATGTC` | `168+1` | `CCCTCTTCGTGATGTT` | 149 | 24.833333333333 |
| `CCTTCTTCGTGATGTT` | `2379+0` | `CCTTCTTCGTGATGTC` | 131 | 21.833333333333 |
| `CCTTCTTCGTGATGTT` | `2379-1` | `CCTTCTTCGTGATGTC` | 121 | 20.166666666667 |
| `ACTTTTCTGTGATGAC` | `2379+0` | `ACTTTTCTGTGATGAC` | 74 | 74.000000000000 |
| `CCTTCTTCGTGATGTC` | `other` | `CCTTCTTCGTGATGTC` | 63 | 4.846153846154 |
| `CCTTCTTCGTGATGTT` | `2380+0` | `CCTTCTTCGTGATGTC` | 59 | 14.750000000000 |
| `CCTTCTTCGTGATGTC` | `other` | `CCTTCTTCGTGATGTT` | 54 | 4.153846153846 |
| `CCTTCTTCGTGATGTT` | `2379-2` | `CCTTCTTCGTGATGTC` | 43 | 14.333333333333 |
| `GCTTCTCTGTGATGAC` | `508+0` | `CCTTCTTCGTGATGTC` | 42 | 14.000000000000 |
| `CCTTCTTCGTGATGTT` | `2379-3` | `CCTTCTTCGTGATGTC` | 42 | 21.000000000000 |
| `CCTTCTTCGTGATGTC` | `341-3` | `CCTTCTTCGTGATGTT` | 40 | 8.000000000000 |
| `CCTTCTTCGTGATGTT` | `other` | `CCCTCTTCGTGATGTT` | 38 | 2.923076923077 |
| `CCCTCTTCGTGATGTT` | `other` | `CCCTCTTCGTGATGTT` | 36 | 1.440000000000 |
| `CCCTCTTCGTGATGTT` | `other` | `CCTTCTTCGTGATGTT` | 32 | 1.280000000000 |
| `CCTTCTTCGTGATGTT` | `other` | `CCTTCTTCGTGATGTC` | 31 | 2.384615384615 |
| `CCTTCTTCGTGATGTT` | `2380+1` | `CCTTCTTCGTGATGTC` | 26 | 8.666666666667 |
| `ACTTTTCTGTGATGAC` | `2380+0` | `ACTTTTCTGTGATGAC` | 24 | 24.000000000000 |

## Family-conditioned shuffle summary

| metric | observed | null_mean | z | empirical_p_right_tail |
|---|---:|---:|---:|---:|
| weighted_top1_accuracy | 0.863220926568 | 0.419381972896 | 99.669068 | 0.000999000999 |
| top_transition_count | 729.000000000000 | 126.643000000000 | 87.243570 | 0.000999000999 |
| mutual_information_bits | 1.700763070072 | 0.409015250227 | 114.079055 | 0.000999000999 |

## Control gates

| gate | status | value | threshold |
|---|---|---|---|
| repeat_family_identified | pass | `ALR/Alpha|Satellite|centr` | `non-empty` |
| dominant_family_coverage | pass | `1.0` | `>=0.80` |
| family_skeleton_scan | warn | `3173` | `>= observed family blocks (3174)` |
| family_conditioned_shuffle_weighted_top1_accuracy | pass | `z=99.66906782055726;p=0.000999000999000999` | `z>=10 and p<=0.01` |
| family_conditioned_shuffle_top_transition_count | pass | `z=87.2435704867108;p=0.000999000999000999` | `z>=10 and p<=0.01` |
| family_conditioned_shuffle_mutual_information_bits | pass | `z=114.07905503611015;p=0.000999000999000999` | `z>=10 and p<=0.01` |
| v09_scoped_claim | pass | `repeat_family_alone_not_sufficient` | `all family-conditioned shuffle gates pass` |

## Scoped interpretation

If `v09_scoped_claim = pass`, the allowed claim is:

```text
RepeatMasker family membership alone does not explain the observed
A/Spacer/B spacer-state automaton.
```

This still does not prove artificial origin. It moves the remaining natural
model from "known repeat family" to a more specific question:

```text
What process inside this repeat family produces the observed deterministic
spacer-state transition grammar?
```

## Output files

```text
cluster_v09_block_repeat_annotations.csv
cluster_v09_repeat_group_summary.csv
cluster_v09_family_skeleton_blocks.csv
cluster_v09_family_spacer_states.csv
cluster_v09_family_transitions.csv
cluster_v09_family_shuffle_summary.csv
cluster_v09_control_gates.csv
cluster_v09_manifest.json
cluster_v09_report.md
```
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v09_final\cluster_v09_repeat_group_summary.csv |
>>   Select-Object -First 20


repeat_key       : ALR/Alpha|Satellite|centr
rep_name         : ALR/Alpha
rep_class        : Satellite
rep_family       : centr
blocks           : 3174
block_fraction   : 1.0
unique_spacers   : 92
top_spacer       : CCTTCTTCGTGATGTC
top_spacer_count : 1140
unique_full47    : 92
start_min        : 22747231
start_max        : 26712566



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v09_final\cluster_v09_family_shuffle_summary.csv


metric                 : weighted_top1_accuracy
observed               : 0.8632209265679168
null_mean              : 0.4193819728963126
null_sd                : 0.004453126364848575
z                      : 99.66906782055726
empirical_p_right_tail : 0.000999000999000999
iterations             : 1000

metric                 : top_transition_count
observed               : 729.0
null_mean              : 126.643
null_sd                : 6.904313941297861
z                      : 87.2435704867108
empirical_p_right_tail : 0.000999000999000999
iterations             : 1000

metric                 : mutual_information_bits
observed               : 1.7007630700720562
null_mean              : 0.409015250226537
null_sd                : 0.011323268933430719
z                      : 114.07905503611015
empirical_p_right_tail : 0.000999000999000999
iterations             : 1000



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v09_final\cluster_v09_control_gates.csv


gate      : repeat_family_identified
status    : pass
value     : ALR/Alpha|Satellite|centr
threshold : non-empty
rationale : Dominant RepeatMasker group assigned to observed blocks.

gate      : dominant_family_coverage
status    : pass
value     : 1.0
threshold : >=0.80
rationale : Most observed blocks must be in one repeat family for family-conditioned test.

gate      : family_skeleton_scan
status    : warn
value     : 3173
threshold : >= observed family blocks (3174)
rationale : Whole-family scan should recover at least observed A/Spacer/B blocks if enabled.

gate      : family_conditioned_shuffle_weighted_top1_accuracy
status    : pass
value     : z=99.66906782055726;p=0.000999000999000999
threshold : z>=10 and p<=0.01
rationale : Observed automaton remains exceptional after conditioning on RepeatMasker family and preserving state
            counts.

gate      : family_conditioned_shuffle_top_transition_count
status    : pass
value     : z=87.2435704867108;p=0.000999000999000999
threshold : z>=10 and p<=0.01
rationale : Observed automaton remains exceptional after conditioning on RepeatMasker family and preserving state
            counts.

gate      : family_conditioned_shuffle_mutual_information_bits
status    : pass
value     : z=114.07905503611015;p=0.000999000999000999
threshold : z>=10 and p<=0.01
rationale : Observed automaton remains exceptional after conditioning on RepeatMasker family and preserving state
            counts.

gate      : v09_scoped_claim
status    : pass
value     : repeat_family_alone_not_sufficient
threshold : all family-conditioned shuffle gates pass
rationale : Release only the scoped claim that RepeatMasker family membership alone does not explain the observed
            automaton.



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v09_final\cluster_v09_family_transitions.csv |
>>   Select-Object -First 40


from_spacer      : CCTTCTTCGTGATGTC
delta_family     : 170+0
to_spacer        : CCCTCTTCGTGATGTT
count            : 729
conditional_rate : 31.695652173913043

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : other
to_spacer        : CCTTCTTCGTGATGTC
count            : 481
conditional_rate : 19.24

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : 168-1
to_spacer        : CCTTCTTCGTGATGTT
count            : 347
conditional_rate : 38.55555555555556

from_spacer      : CCTTCTTCGTGATGTC
delta_family     : 168+1
to_spacer        : CCCTCTTCGTGATGTT
count            : 149
conditional_rate : 24.833333333333332

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 2379+0
to_spacer        : CCTTCTTCGTGATGTC
count            : 131
conditional_rate : 21.833333333333332

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 2379-1
to_spacer        : CCTTCTTCGTGATGTC
count            : 121
conditional_rate : 20.166666666666668

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : 2379+0
to_spacer        : ACTTTTCTGTGATGAC
count            : 74
conditional_rate : 74.0

from_spacer      : CCTTCTTCGTGATGTC
delta_family     : other
to_spacer        : CCTTCTTCGTGATGTC
count            : 63
conditional_rate : 4.846153846153846

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 2380+0
to_spacer        : CCTTCTTCGTGATGTC
count            : 59
conditional_rate : 14.75

from_spacer      : CCTTCTTCGTGATGTC
delta_family     : other
to_spacer        : CCTTCTTCGTGATGTT
count            : 54
conditional_rate : 4.153846153846154

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 2379-2
to_spacer        : CCTTCTTCGTGATGTC
count            : 43
conditional_rate : 14.333333333333334

from_spacer      : GCTTCTCTGTGATGAC
delta_family     : 508+0
to_spacer        : CCTTCTTCGTGATGTC
count            : 42
conditional_rate : 14.0

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 2379-3
to_spacer        : CCTTCTTCGTGATGTC
count            : 42
conditional_rate : 21.0

from_spacer      : CCTTCTTCGTGATGTC
delta_family     : 341-3
to_spacer        : CCTTCTTCGTGATGTT
count            : 40
conditional_rate : 8.0

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : other
to_spacer        : CCCTCTTCGTGATGTT
count            : 38
conditional_rate : 2.923076923076923

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : other
to_spacer        : CCCTCTTCGTGATGTT
count            : 36
conditional_rate : 1.44

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : other
to_spacer        : CCTTCTTCGTGATGTT
count            : 32
conditional_rate : 1.28

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : other
to_spacer        : CCTTCTTCGTGATGTC
count            : 31
conditional_rate : 2.3846153846153846

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 2380+1
to_spacer        : CCTTCTTCGTGATGTC
count            : 26
conditional_rate : 8.666666666666666

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : 2380+0
to_spacer        : ACTTTTCTGTGATGAC
count            : 24
conditional_rate : 24.0

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : other
to_spacer        : GCTTCTCTGTGATGAC
count            : 24
conditional_rate : 0.96

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : other
to_spacer        : GCTTCTCTGTGATGAC
count            : 22
conditional_rate : 1.6923076923076923

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : other
to_spacer        : ACTTTTCTGTGATGAC
count            : 20
conditional_rate : 2.857142857142857

from_spacer      : CCTTCTTCGTGATGTC
delta_family     : 170+0
to_spacer        : CCTTCTTCGTGATGTT
count            : 20
conditional_rate : 0.8695652173913043

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : 2379+0
to_spacer        : CCTTCTTCGTGATGTC
count            : 17
conditional_rate : 8.5

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 508+1
to_spacer        : ACTTTTCTGTGATGAC
count            : 13
conditional_rate : 13.0

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : other
to_spacer        : CCTTCTTCGTGATGTT
count            : 12
conditional_rate : 0.9230769230769231

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : 2380+1
to_spacer        : ACTTTTCTGTGATGAC
count            : 10
conditional_rate : 5.0

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : 2379-1
to_spacer        : CCTTCTTCGTGATGTC
count            : 10
conditional_rate : 2.5

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : 2379-1
to_spacer        : ACTTTTCTGTGATGAC
count            : 9
conditional_rate : 9.0

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : other
to_spacer        : ACTCCTTTGTGATGTT
count            : 9
conditional_rate : 1.2857142857142858

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : 2380+0
to_spacer        : CCTTCTTCGTGATGTC
count            : 8
conditional_rate : 2.6666666666666665

from_spacer      : CCTTCTTCGTGATGTC
delta_family     : 171+0
to_spacer        : CCCTCTTCGTGATGTT
count            : 8
conditional_rate : 4.0

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : 168+0
to_spacer        : CCTTCTTCGTGATGTT
count            : 8
conditional_rate : 8.0

from_spacer      : ACTCCTTTGTGATGTT
delta_family     : 508+2
to_spacer        : CCTTCTTCGTGATGTT
count            : 8
conditional_rate : 4.0

from_spacer      : CCCTCTTCGTGATGTT
delta_family     : 168-1
to_spacer        : CCCTCTTCGTGATGTT
count            : 7
conditional_rate : 0.7777777777777778

from_spacer      : CCTTCTTCGTGGTGTC
delta_family     : 170+0
to_spacer        : CCCTCTTCGTGATGTT
count            : 7
conditional_rate : 7.0

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : other
to_spacer        : CCTTCTTCGTGATGTT
count            : 5
conditional_rate : 0.7142857142857143

from_spacer      : ACTTTTCTGTGATGAC
delta_family     : 2380+2
to_spacer        : ACTTTTCTGTGATGAC
count            : 5
conditional_rate : 5.0

from_spacer      : CCTTCTTCGTGATGTT
delta_family     : 2380+2
to_spacer        : CCTTCTTCGTGATGTC
count            : 5
conditional_rate : 5.0



PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v09_final\* `
>>   -DestinationPath .\Decode_chr17_v09_final_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun>


--------------------------------------------

PS H:\kdna_opencl_with_inspector_krun> .\scripts\kalyx_v091_ratefix_hor_v10.ps1 `
>>   -SequenceDir .\Decode_chr17_v04_real `
>>   -V09Dir .\Decode_chr17_v09_final `
>>   -OutDir .\Decode_chr17_v10_final `
>>   -Iterations 1000 `
>>   -HorPeriod 2380 `
>>   -HorSlots 14
KALYX v0.9.1 + v1.0 complete: Decode_chr17_v10_final
  blocks_used=3174 hor_period=2380 hor_slots=14
  claim_status=hor_conditioned_repeat_model_not_sufficient
  report=Decode_chr17_v10_final\cluster_v10_report.md


PS H:\kdna_opencl_with_inspector_krun> Get-Content .\Decode_chr17_v10_final\cluster_v10_report.md
# KALYX v0.9.1 Rate-Fix + v1.0 Alpha-Satellite/HOR-conditioned Automaton Test

## Boundary

Dieser Lauf korrigiert zuerst die v0.9-Transition-Rates und testet danach,
ob der Spacer-State-Automat auch nach Konditionierung auf eine grobe HOR-Struktur
außergewöhnlich bleibt. Er beweist keinen natürlichen oder künstlichen Ursprung.

## Inputs

- v0.4 sequence dir: `Decode_chr17_v04_real`
- v0.9 repeat-family dir: `Decode_chr17_v09_final`
- target rank: `3`
- blocks used: `3174`
- dominant repeat group: `ALR/Alpha|Satellite|centr`
- HOR period: `2380`
- HOR slots: `14`
- iterations: `1000`

## v0.9.1 Rate-Fix

Die alte Spalte `conditional_rate` aus v0.9 wird ersetzt durch:

```text
conditional_rate_from_state
conditional_rate_from_state_and_delta
```

Beide liegen per Definition in `[0,1]`.

## Top corrected transitions

| from | delta | to | count | rate_from_state | rate_from_state_and_delta |
|---|---|---|---:|---:|---:|
| `CCTTCTTCGTGATGTC` | `170+0` | `CCCTCTTCGTGATGTT` | 729 | 0.639473684211 | 0.934615384615 |
| `CCCTCTTCGTGATGTT` | `other` | `CCTTCTTCGTGATGTC` | 481 | 0.468354430380 | 0.799003322259 |
| `CCCTCTTCGTGATGTT` | `168-1` | `CCTTCTTCGTGATGTT` | 347 | 0.337877312561 | 0.955922865014 |
| `CCTTCTTCGTGATGTC` | `168+1` | `CCCTCTTCGTGATGTT` | 149 | 0.130701754386 | 0.961290322581 |
| `CCTTCTTCGTGATGTT` | `2379+0` | `CCTTCTTCGTGATGTC` | 131 | 0.222789115646 | 0.949275362319 |
| `CCTTCTTCGTGATGTT` | `2379-1` | `CCTTCTTCGTGATGTC` | 121 | 0.205782312925 | 0.952755905512 |
| `ACTTTTCTGTGATGAC` | `2379+0` | `ACTTTTCTGTGATGAC` | 74 | 0.437869822485 | 1.000000000000 |
| `CCTTCTTCGTGATGTC` | `other` | `CCTTCTTCGTGATGTC` | 63 | 0.055263157895 | 0.470149253731 |
| `CCTTCTTCGTGATGTT` | `2380+0` | `CCTTCTTCGTGATGTC` | 59 | 0.100340136054 | 0.921875000000 |
| `CCTTCTTCGTGATGTC` | `other` | `CCTTCTTCGTGATGTT` | 54 | 0.047368421053 | 0.402985074627 |
| `CCTTCTTCGTGATGTT` | `2379-2` | `CCTTCTTCGTGATGTC` | 43 | 0.073129251701 | 0.955555555556 |
| `GCTTCTCTGTGATGAC` | `508+0` | `CCTTCTTCGTGATGTC` | 42 | 0.807692307692 | 0.954545454545 |
| `CCTTCTTCGTGATGTT` | `2379-3` | `CCTTCTTCGTGATGTC` | 42 | 0.071428571429 | 0.976744186047 |
| `CCTTCTTCGTGATGTC` | `341-3` | `CCTTCTTCGTGATGTT` | 40 | 0.035087719298 | 0.909090909091 |
| `CCTTCTTCGTGATGTT` | `other` | `CCCTCTTCGTGATGTT` | 38 | 0.064625850340 | 0.339285714286 |
| `CCCTCTTCGTGATGTT` | `other` | `CCCTCTTCGTGATGTT` | 36 | 0.035053554041 | 0.059800664452 |
| `CCCTCTTCGTGATGTT` | `other` | `CCTTCTTCGTGATGTT` | 32 | 0.031158714703 | 0.053156146179 |
| `CCTTCTTCGTGATGTT` | `other` | `CCTTCTTCGTGATGTC` | 31 | 0.052721088435 | 0.276785714286 |
| `CCTTCTTCGTGATGTT` | `2380+1` | `CCTTCTTCGTGATGTC` | 26 | 0.044217687075 | 0.896551724138 |
| `ACTTTTCTGTGATGAC` | `2380+0` | `ACTTTTCTGTGATGAC` | 24 | 0.142011834320 | 1.000000000000 |
| `CCCTCTTCGTGATGTT` | `other` | `GCTTCTCTGTGATGAC` | 24 | 0.023369036027 | 0.039867109635 |
| `CCTTCTTCGTGATGTT` | `other` | `GCTTCTCTGTGATGAC` | 22 | 0.037414965986 | 0.196428571429 |
| `ACTTTTCTGTGATGAC` | `other` | `ACTTTTCTGTGATGAC` | 20 | 0.118343195266 | 0.487804878049 |
| `CCTTCTTCGTGATGTC` | `170+0` | `CCTTCTTCGTGATGTT` | 20 | 0.017543859649 | 0.025641025641 |
| `CCCTCTTCGTGATGTT` | `2379+0` | `CCTTCTTCGTGATGTC` | 17 | 0.016553067186 | 0.944444444444 |

## v1.0 HOR-slot summary

| hor_slot | blocks | unique_states | top_state | top_count | top_rate |
|---:|---:|---:|---|---:|---:|
| 0 | 257 | 13 | `CCTTCTTCGTGATGTC` | 77 | 0.299610894942 |
| 1 | 259 | 26 | `CCTTCTTCGTGATGTC` | 80 | 0.308880308880 |
| 2 | 221 | 13 | `CCTTCTTCGTGATGTC` | 87 | 0.393665158371 |
| 3 | 217 | 13 | `CCTTCTTCGTGATGTC` | 87 | 0.400921658986 |
| 4 | 234 | 18 | `CCTTCTTCGTGATGTC` | 89 | 0.380341880342 |
| 5 | 199 | 15 | `CCCTCTTCGTGATGTT` | 77 | 0.386934673367 |
| 6 | 189 | 17 | `CCTTCTTCGTGATGTC` | 74 | 0.391534391534 |
| 7 | 257 | 19 | `CCTTCTTCGTGATGTC` | 77 | 0.299610894942 |
| 8 | 225 | 22 | `CCTTCTTCGTGATGTC` | 85 | 0.377777777778 |
| 9 | 227 | 14 | `CCTTCTTCGTGATGTC` | 81 | 0.356828193833 |
| 10 | 207 | 17 | `CCTTCTTCGTGATGTC` | 79 | 0.381642512077 |
| 11 | 216 | 9 | `CCTTCTTCGTGATGTC` | 86 | 0.398148148148 |
| 12 | 224 | 17 | `CCTTCTTCGTGATGTC` | 89 | 0.397321428571 |
| 13 | 242 | 23 | `CCCTCTTCGTGATGTT` | 85 | 0.351239669421 |

## HOR-conditioned shuffle summary

| metric | observed | null_mean | z | empirical_p_right_tail |
|---|---:|---:|---:|---:|
| weighted_top1_accuracy | 0.8632209265679168 | 0.41967381027418843 | 103.45986042819017 | 0.000999000999000999 |
| top_transition_count | 729.0 | 127.462 | 87.61056976322112 | 0.000999000999000999 |
| mutual_information_to_from_delta_bits | 1.7007630700720562 | 0.41481465128694417 | 105.11555700277053 | 0.000999000999000999 |
| max_same_state_run_length | 140.0 | 7.824 | 111.98846848001088 | 0.000999000999000999 |
| states | 3174.0 | 3174.0 | 0.0 | 1.0 |
| transitions | 3173.0 | 3173.0 | 0.0 | 1.0 |

## Control gates

| gate | status | value | threshold |
|---|---|---|---|
| v091_rate_fix | pass | bounded_rates | all rates in [0,1] |
| hor_conditioned_weighted_top1_accuracy | pass | z=103.45986042819017;p=0.000999000999000999 | z>=5.0;p<=0.01 |
| hor_conditioned_mutual_information_to_from_delta_bits | pass | z=105.11555700277053;p=0.000999000999000999 | z>=5.0;p<=0.01 |
| hor_conditioned_top_transition_count | pass | z=87.61056976322112;p=0.000999000999000999 | z>=5.0;p<=0.01 |
| v10_scoped_claim | pass | HOR-conditioned repeat-family null | all v1.0 gates pass |

## Scoped claim

If `v10_scoped_claim = pass`, the allowed claim is:

```text
Die grobe Alpha-Satellite/HOR-Slot-Struktur allein erklärt den beobachteten
A/Spacer/B-Spacer-State-Automaten nicht.
```

This is not an artificial-origin proof. It narrows the remaining natural model
from "RepeatMasker family" to a more specific HOR/Alpha-Satellite mechanism that
must reproduce the same state grammar.

## Output files

```text
cluster_v091_fixed_transitions.csv
cluster_v091_state_outgoing_summary.csv
cluster_v10_hor_slots.csv
cluster_v10_hor_slot_summary.csv
cluster_v10_hor_conditioned_transitions.csv
cluster_v10_hor_shuffle_summary.csv
cluster_v10_control_gates.csv
cluster_v10_manifest.json
cluster_v10_report.md
```
PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v10_final\cluster_v091_fixed_transitions.csv |
>>   Select-Object -First 40


from_spacer                           : CCTTCTTCGTGATGTC
delta_family                          : 170+0
to_spacer                             : CCCTCTTCGTGATGTT
count                                 : 729
from_state_total                      : 1140
from_state_delta_total                : 780
delta_total                           : 825
conditional_rate_from_state           : 0.6394736842105263
conditional_rate_from_state_and_delta : 0.9346153846153846
share_of_delta_family                 : 0.8836363636363637

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 481
from_state_total                      : 1027
from_state_delta_total                : 602
delta_total                           : 961
conditional_rate_from_state           : 0.46835443037974683
conditional_rate_from_state_and_delta : 0.7990033222591362
share_of_delta_family                 : 0.5005202913631633

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : 168-1
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 347
from_state_total                      : 1027
from_state_delta_total                : 363
delta_total                           : 398
conditional_rate_from_state           : 0.33787731256085685
conditional_rate_from_state_and_delta : 0.9559228650137741
share_of_delta_family                 : 0.871859296482412

from_spacer                           : CCTTCTTCGTGATGTC
delta_family                          : 168+1
to_spacer                             : CCCTCTTCGTGATGTT
count                                 : 149
from_state_total                      : 1140
from_state_delta_total                : 155
delta_total                           : 161
conditional_rate_from_state           : 0.1307017543859649
conditional_rate_from_state_and_delta : 0.9612903225806452
share_of_delta_family                 : 0.9254658385093167

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 2379+0
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 131
from_state_total                      : 588
from_state_delta_total                : 138
delta_total                           : 234
conditional_rate_from_state           : 0.2227891156462585
conditional_rate_from_state_and_delta : 0.9492753623188406
share_of_delta_family                 : 0.5598290598290598

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 2379-1
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 121
from_state_total                      : 588
from_state_delta_total                : 127
delta_total                           : 154
conditional_rate_from_state           : 0.20578231292517007
conditional_rate_from_state_and_delta : 0.952755905511811
share_of_delta_family                 : 0.7857142857142857

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : 2379+0
to_spacer                             : ACTTTTCTGTGATGAC
count                                 : 74
from_state_total                      : 169
from_state_delta_total                : 74
delta_total                           : 234
conditional_rate_from_state           : 0.4378698224852071
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.3162393162393162

from_spacer                           : CCTTCTTCGTGATGTC
delta_family                          : other
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 63
from_state_total                      : 1140
from_state_delta_total                : 134
delta_total                           : 961
conditional_rate_from_state           : 0.05526315789473684
conditional_rate_from_state_and_delta : 0.4701492537313433
share_of_delta_family                 : 0.06555671175858481

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 2380+0
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 59
from_state_total                      : 588
from_state_delta_total                : 64
delta_total                           : 107
conditional_rate_from_state           : 0.10034013605442177
conditional_rate_from_state_and_delta : 0.921875
share_of_delta_family                 : 0.5514018691588785

from_spacer                           : CCTTCTTCGTGATGTC
delta_family                          : other
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 54
from_state_total                      : 1140
from_state_delta_total                : 134
delta_total                           : 961
conditional_rate_from_state           : 0.04736842105263158
conditional_rate_from_state_and_delta : 0.40298507462686567
share_of_delta_family                 : 0.05619146722164412

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 2379-2
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 43
from_state_total                      : 588
from_state_delta_total                : 45
delta_total                           : 50
conditional_rate_from_state           : 0.07312925170068027
conditional_rate_from_state_and_delta : 0.9555555555555556
share_of_delta_family                 : 0.86

from_spacer                           : GCTTCTCTGTGATGAC
delta_family                          : 508+0
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 42
from_state_total                      : 52
from_state_delta_total                : 44
delta_total                           : 45
conditional_rate_from_state           : 0.8076923076923077
conditional_rate_from_state_and_delta : 0.9545454545454546
share_of_delta_family                 : 0.9333333333333333

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 2379-3
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 42
from_state_total                      : 588
from_state_delta_total                : 43
delta_total                           : 48
conditional_rate_from_state           : 0.07142857142857142
conditional_rate_from_state_and_delta : 0.9767441860465116
share_of_delta_family                 : 0.875

from_spacer                           : CCTTCTTCGTGATGTC
delta_family                          : 341-3
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 40
from_state_total                      : 1140
from_state_delta_total                : 44
delta_total                           : 46
conditional_rate_from_state           : 0.03508771929824561
conditional_rate_from_state_and_delta : 0.9090909090909091
share_of_delta_family                 : 0.8695652173913043

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : CCCTCTTCGTGATGTT
count                                 : 38
from_state_total                      : 588
from_state_delta_total                : 112
delta_total                           : 961
conditional_rate_from_state           : 0.06462585034013606
conditional_rate_from_state_and_delta : 0.3392857142857143
share_of_delta_family                 : 0.03954214360041623

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : CCCTCTTCGTGATGTT
count                                 : 36
from_state_total                      : 1027
from_state_delta_total                : 602
delta_total                           : 961
conditional_rate_from_state           : 0.035053554040895815
conditional_rate_from_state_and_delta : 0.059800664451827246
share_of_delta_family                 : 0.037460978147762745

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 32
from_state_total                      : 1027
from_state_delta_total                : 602
delta_total                           : 961
conditional_rate_from_state           : 0.0311587147030185
conditional_rate_from_state_and_delta : 0.053156146179401995
share_of_delta_family                 : 0.03329864724245578

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 31
from_state_total                      : 588
from_state_delta_total                : 112
delta_total                           : 961
conditional_rate_from_state           : 0.05272108843537415
conditional_rate_from_state_and_delta : 0.2767857142857143
share_of_delta_family                 : 0.03225806451612903

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 2380+1
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 26
from_state_total                      : 588
from_state_delta_total                : 29
delta_total                           : 46
conditional_rate_from_state           : 0.04421768707482993
conditional_rate_from_state_and_delta : 0.896551724137931
share_of_delta_family                 : 0.5652173913043478

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : 2380+0
to_spacer                             : ACTTTTCTGTGATGAC
count                                 : 24
from_state_total                      : 169
from_state_delta_total                : 24
delta_total                           : 107
conditional_rate_from_state           : 0.14201183431952663
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.22429906542056074

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : GCTTCTCTGTGATGAC
count                                 : 24
from_state_total                      : 1027
from_state_delta_total                : 602
delta_total                           : 961
conditional_rate_from_state           : 0.023369036027263874
conditional_rate_from_state_and_delta : 0.03986710963455149
share_of_delta_family                 : 0.02497398543184183

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : GCTTCTCTGTGATGAC
count                                 : 22
from_state_total                      : 588
from_state_delta_total                : 112
delta_total                           : 961
conditional_rate_from_state           : 0.03741496598639456
conditional_rate_from_state_and_delta : 0.19642857142857142
share_of_delta_family                 : 0.022892819979188347

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : other
to_spacer                             : ACTTTTCTGTGATGAC
count                                 : 20
from_state_total                      : 169
from_state_delta_total                : 41
delta_total                           : 961
conditional_rate_from_state           : 0.11834319526627218
conditional_rate_from_state_and_delta : 0.4878048780487805
share_of_delta_family                 : 0.02081165452653486

from_spacer                           : CCTTCTTCGTGATGTC
delta_family                          : 170+0
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 20
from_state_total                      : 1140
from_state_delta_total                : 780
delta_total                           : 825
conditional_rate_from_state           : 0.017543859649122806
conditional_rate_from_state_and_delta : 0.02564102564102564
share_of_delta_family                 : 0.024242424242424242

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : 2379+0
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 17
from_state_total                      : 1027
from_state_delta_total                : 18
delta_total                           : 234
conditional_rate_from_state           : 0.016553067185978577
conditional_rate_from_state_and_delta : 0.9444444444444444
share_of_delta_family                 : 0.07264957264957266

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 508+1
to_spacer                             : ACTTTTCTGTGATGAC
count                                 : 13
from_state_total                      : 588
from_state_delta_total                : 13
delta_total                           : 18
conditional_rate_from_state           : 0.022108843537414966
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.7222222222222222

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : other
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 12
from_state_total                      : 588
from_state_delta_total                : 112
delta_total                           : 961
conditional_rate_from_state           : 0.02040816326530612
conditional_rate_from_state_and_delta : 0.10714285714285714
share_of_delta_family                 : 0.012486992715920915

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : 2380+1
to_spacer                             : ACTTTTCTGTGATGAC
count                                 : 10
from_state_total                      : 169
from_state_delta_total                : 11
delta_total                           : 46
conditional_rate_from_state           : 0.05917159763313609
conditional_rate_from_state_and_delta : 0.9090909090909091
share_of_delta_family                 : 0.21739130434782608

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : 2379-1
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 10
from_state_total                      : 1027
from_state_delta_total                : 13
delta_total                           : 154
conditional_rate_from_state           : 0.009737098344693282
conditional_rate_from_state_and_delta : 0.7692307692307693
share_of_delta_family                 : 0.06493506493506493

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : 2379-1
to_spacer                             : ACTTTTCTGTGATGAC
count                                 : 9
from_state_total                      : 169
from_state_delta_total                : 9
delta_total                           : 154
conditional_rate_from_state           : 0.05325443786982249
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.05844155844155844

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : other
to_spacer                             : ACTCCTTTGTGATGTT
count                                 : 9
from_state_total                      : 169
from_state_delta_total                : 41
delta_total                           : 961
conditional_rate_from_state           : 0.05325443786982249
conditional_rate_from_state_and_delta : 0.21951219512195122
share_of_delta_family                 : 0.009365244536940686

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : 2380+0
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 8
from_state_total                      : 1027
from_state_delta_total                : 10
delta_total                           : 107
conditional_rate_from_state           : 0.007789678675754625
conditional_rate_from_state_and_delta : 0.8
share_of_delta_family                 : 0.07476635514018691

from_spacer                           : CCTTCTTCGTGATGTC
delta_family                          : 171+0
to_spacer                             : CCCTCTTCGTGATGTT
count                                 : 8
from_state_total                      : 1140
from_state_delta_total                : 9
delta_total                           : 12
conditional_rate_from_state           : 0.007017543859649123
conditional_rate_from_state_and_delta : 0.8888888888888888
share_of_delta_family                 : 0.6666666666666666

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : 168+0
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 8
from_state_total                      : 1027
from_state_delta_total                : 8
delta_total                           : 15
conditional_rate_from_state           : 0.007789678675754625
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.5333333333333333

from_spacer                           : ACTCCTTTGTGATGTT
delta_family                          : 508+2
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 8
from_state_total                      : 13
from_state_delta_total                : 11
delta_total                           : 15
conditional_rate_from_state           : 0.6153846153846154
conditional_rate_from_state_and_delta : 0.7272727272727273
share_of_delta_family                 : 0.5333333333333333

from_spacer                           : CCCTCTTCGTGATGTT
delta_family                          : 168-1
to_spacer                             : CCCTCTTCGTGATGTT
count                                 : 7
from_state_total                      : 1027
from_state_delta_total                : 363
delta_total                           : 398
conditional_rate_from_state           : 0.006815968841285297
conditional_rate_from_state_and_delta : 0.01928374655647383
share_of_delta_family                 : 0.017587939698492462

from_spacer                           : CCTTCTTCGTGGTGTC
delta_family                          : 170+0
to_spacer                             : CCCTCTTCGTGATGTT
count                                 : 7
from_state_total                      : 7
from_state_delta_total                : 7
delta_total                           : 825
conditional_rate_from_state           : 1.0
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.008484848484848486

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : other
to_spacer                             : CCTTCTTCGTGATGTT
count                                 : 5
from_state_total                      : 169
from_state_delta_total                : 41
delta_total                           : 961
conditional_rate_from_state           : 0.029585798816568046
conditional_rate_from_state_and_delta : 0.12195121951219512
share_of_delta_family                 : 0.005202913631633715

from_spacer                           : ACTTTTCTGTGATGAC
delta_family                          : 2380+2
to_spacer                             : ACTTTTCTGTGATGAC
count                                 : 5
from_state_total                      : 169
from_state_delta_total                : 5
delta_total                           : 12
conditional_rate_from_state           : 0.029585798816568046
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.4166666666666667

from_spacer                           : CCTTCTTCGTGATGTT
delta_family                          : 2380+2
to_spacer                             : CCTTCTTCGTGATGTC
count                                 : 5
from_state_total                      : 588
from_state_delta_total                : 5
delta_total                           : 12
conditional_rate_from_state           : 0.008503401360544218
conditional_rate_from_state_and_delta : 1.0
share_of_delta_family                 : 0.4166666666666667



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v10_final\cluster_v10_hor_shuffle_summary.csv


metric                 : weighted_top1_accuracy
observed               : 0.8632209265679168
null_mean              : 0.41967381027418843
null_sd                : 0.004287142032262718
z                      : 103.45986042819017
empirical_p_right_tail : 0.000999000999000999
iterations             : 1000
null_model             : shuffle_states_within_hor_slot_period_2380_slots_14

metric                 : top_transition_count
observed               : 729.0
null_mean              : 127.462
null_sd                : 6.866043693423455
z                      : 87.61056976322112
empirical_p_right_tail : 0.000999000999000999
iterations             : 1000
null_model             : shuffle_states_within_hor_slot_period_2380_slots_14

metric                 : mutual_information_to_from_delta_bits
observed               : 1.7007630700720562
null_mean              : 0.41481465128694417
null_sd                : 0.01223366412596014
z                      : 105.11555700277053
empirical_p_right_tail : 0.000999000999000999
iterations             : 1000
null_model             : shuffle_states_within_hor_slot_period_2380_slots_14

metric                 : max_same_state_run_length
observed               : 140.0
null_mean              : 7.824
null_sd                : 1.180264377163015
z                      : 111.98846848001088
empirical_p_right_tail : 0.000999000999000999
iterations             : 1000
null_model             : shuffle_states_within_hor_slot_period_2380_slots_14

metric                 : states
observed               : 3174.0
null_mean              : 3174.0
null_sd                : 0.0
z                      : 0.0
empirical_p_right_tail : 1.0
iterations             : 1000
null_model             : shuffle_states_within_hor_slot_period_2380_slots_14

metric                 : transitions
observed               : 3173.0
null_mean              : 3173.0
null_sd                : 0.0
z                      : 0.0
empirical_p_right_tail : 1.0
iterations             : 1000
null_model             : shuffle_states_within_hor_slot_period_2380_slots_14



PS H:\kdna_opencl_with_inspector_krun> Import-Csv .\Decode_chr17_v10_final\cluster_v10_control_gates.csv


gate      : v091_rate_fix
status    : pass
value     : bounded_rates
threshold : all rates in [0,1]
rationale : v0.9.1 corrected transition denominators.

gate      : hor_conditioned_weighted_top1_accuracy
status    : pass
value     : z=103.45986042819017;p=0.000999000999000999
threshold : z>=5.0;p<=0.01
rationale : Observed automaton remains stronger than HOR-slot-conditioned shuffle.

gate      : hor_conditioned_mutual_information_to_from_delta_bits
status    : pass
value     : z=105.11555700277053;p=0.000999000999000999
threshold : z>=5.0;p<=0.01
rationale : Observed automaton remains stronger than HOR-slot-conditioned shuffle.

gate      : hor_conditioned_top_transition_count
status    : pass
value     : z=87.61056976322112;p=0.000999000999000999
threshold : z>=5.0;p<=0.01
rationale : Observed automaton remains stronger than HOR-slot-conditioned shuffle.

gate      : v10_scoped_claim
status    : pass
value     : HOR-conditioned repeat-family null
threshold : all v1.0 gates pass
rationale : If pass: HOR-slot-conditioned repeat family structure alone is not sufficient.



PS H:\kdna_opencl_with_inspector_krun> Compress-Archive `
>>   -Path .\Decode_chr17_v10_final\* `
>>   -DestinationPath .\Decode_chr17_v10_final_outputs.zip `
>>   -Force
PS H:\kdna_opencl_with_inspector_krun>