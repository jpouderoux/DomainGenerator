#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkExtentRCBPartitioner.h>
#include <vtkExtentTranslator.h>
#include <vtkIdTypeArray.h>
#include <vtkLongLongArray.h>
#include <vtkMPIController.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkTimerLog.h>
#include <vtkXMLPPolyDataReader.h>
#include <vtkXMLPPolyDataWriter.h>

#include <algorithm>
#include <sstream>
 
vtkIdType NumberOfGenerators = 100000;

void orderedPrint(vtkMPIController* controller, const char* str)
{
  for (int i = 0; i < controller->GetNumberOfProcesses(); i++)
    {
    if (controller->GetLocalProcessId() == i)
      {
      cout << "[Rank " << i << "] " << str << endl;
      }
    controller->Barrier();
    }
}

void pmain(vtkMultiProcessController *controllr, void *userData)
{
  const double ratio = 1000000.0;
  
  vtkMPIController* controller = vtkMPIController::SafeDownCast(controllr);  
  int rank = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  
  // Number of points to generate per rank
  vtkIdType numGens = NumberOfGenerators / numProcs;
  
  // compute rank subdomain extents
  double globalExtentDouble[4];
  globalExtentDouble[0] = 0.0;
  globalExtentDouble[1] = 1.0;
  globalExtentDouble[2] = 0.0;
  globalExtentDouble[3] = 1.0;
    
  /*
  vtkNew<vtkExtentRCBPartitioner> partitioner;
  partitioner->SetNumberOfPartitions(numProcs);
  partitioner->SetGlobalExtent(0, 10, 0, 10, 0, 1);
  partitioner->Partition();
  int extent[6];
  partitioner->GetPartitionExtent(rank, extent);
  cout << "Extent " << rank
       << " " << extent[0] << " " << extent[1]
       << " " << extent[2] << " " << extent[3]
       << " " << extent[4] << " " << extent[5]
       << endl;
       */
  
  // conversion from double to int
  int globalExtentInt[4];
  for (int i = 0; i < 4; i++)
    {
    globalExtentInt[i] = (int)(globalExtentDouble[i]*ratio);
    }
  globalExtentInt[4] = 0;
  globalExtentInt[5] = 1;
  // partition of the global extent
  vtkNew<vtkExtentTranslator> translator;
  translator->SetWholeExtent(globalExtentInt);
  translator->SetPiece(rank);
  translator->SetNumberOfPieces(numProcs);
  translator->PieceToExtent();
  int resultExtentInt[6];
  translator->GetExtent(resultExtentInt);
  // conversion from int to double
  double resultExtentDouble[4];
  for (int i = 0; i < 4; i++)
    {
    resultExtentDouble[i] = (double)resultExtentInt[i]/ratio;
    }
  
  cout << "Extent " << rank << " :"
       << " xmin=" << resultExtentDouble[0] << " xmax=" << resultExtentDouble[1]
       << " ymin=" << resultExtentDouble[2] << " ymax=" << resultExtentDouble[3]
       << endl;
  
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numGens);  

  vtkNew<vtkCellArray> verts;
  verts->Allocate(numGens * 2);
  
  vtkNew<vtkLongLongArray> originalIds;
  originalIds->SetName("vtkOriginalCellIds");
  originalIds->SetNumberOfValues(numGens);  
  
  std::srand(time(0));
  for (vtkIdType i = 0; i < numGens; i++)
    {
    double randX = std::rand()/RAND_MAX;
    double X = resultExtentDouble[0] + randX * (resultExtentDouble[1]-resultExtentDouble[0]);
    double randY = std::rand()/RAND_MAX;
    double Y = resultExtentDouble[2] + randY * (resultExtentDouble[3]-resultExtentDouble[2]);
    points->SetPoint(i, X, Y, 0);
    verts->InsertNextCell(1);
    verts->InsertCellPoint(i);
    originalIds->SetValue(i, numGens * rank + i);
    }

  vtkNew<vtkPolyData> pd;
  pd->SetVerts(verts.GetPointer());
  pd->SetPoints(points.GetPointer());
  pd->GetCellData()->AddArray(originalIds.GetPointer());

  vtkNew<vtkXMLPPolyDataWriter> writer;    
  writer->SetInputData(pd.GetPointer());
  writer->SetNumberOfPieces(numProcs);
  writer->SetStartPiece(rank);
  writer->SetEndPiece(rank);
  writer->SetWriteSummaryFile(rank == 0);
  writer->SetFileName("result.pvtp");
  writer->Write();
}

int main(int argc, char** argv)
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  // Récupérer le nombre de générateurs
  controller->SetSingleMethod(pmain, 0);
  controller->SingleMethodExecute();
  controller->Finalize();
}
