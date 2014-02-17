#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkIdTypeArray.h>
#include <vtkLongLongArray.h>
#include <vtkMPIController.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkTimerLog.h>
#include <vtkXMLPPolyDataWriter.h>

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
  vtkMPIController* controller = vtkMPIController::SafeDownCast(controllr);  
  int rank = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  
  // Number of points to generate per rank
  vtkIdType numGens = NumberOfGenerators / numProcs;
  
  // TODO : compute rank subdomain extents
  vtkNew<vtkPoints> points;
  points->SetDataTypeAsDouble();
  points->SetNumberOfPoints(numGens);  

  vtkNew<vtkCellArray> verts;
  verts->Allocate(numGens * 2);
  
  vtkNew<vtkLongLongArray> originalIds;
  originalsIds->SetName(vtkOriginalCellIds);
  originalsIds->SetNumberOfValues(numGens);  
  
  for (vtkIdType i = 0; i < numGens; i++)
    {
    double X, Y;
    // TODO compute X & Y
    points->SetPoint(i, X, Y, 0);
    verts->InsertNextCell(1);
    verts->InsertCellPoint(i);    
    originalsIds->SetValue(i, numGens * rank + i);
    }
    
  vtkNew<vtkPolyData> pd;
  pd->SetVerts(verts.GetPointer());
  pd->SetPoints(points.GetPointer());
  od->GetCellData()->AddArray(originalIds.GetPointer());
     
  vtkNew<vtkXMLPPolyDataWriter> writer;    
  writer->SetInputData(pd.GetPointer());
  writer->SetNumberOfPieces(numProcs);
  writer->SetStartPiece(rank);
  writer->SetEndPiece(rank);
  writer->SetWriteSummaryFile(rank == 0);
  writer->Writer();  
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
