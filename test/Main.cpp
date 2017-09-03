#include <stdio.h>
#include <stdint.h>

#include "dejson.h"
#include "RetroAchievements.h"

int main(int argc, const char* argv[])
{
  if (argc != 2)
  {
    return 1;
  }

  uint8_t json[65536];
  uint8_t buffer[65536];

  {
    FILE* file = fopen(argv[1], "rb");
    int length = fread((void*)json, 1, sizeof(json), file);
    fclose(file);
    json[length] = 0;
  }

  {
    size_t size;
    int res = dejson_get_size(&size, g_MetaPatch.name_hash, json);

    if (res != DEJSON_OK)
    {
      printf("Error: %d\n", res);
      return 1;
    }

    res = dejson_deserialize((void*)buffer, g_MetaPatch.name_hash, json);

    if (res != DEJSON_OK)
    {
      printf("Error: %d\n", res);
      return 1;
    }

    FILE* file = fopen("data.bin", "wb");
    fwrite((void*)buffer, 1, size, file);
    fclose(file);
  }
  
  Patch* patch = (Patch*)buffer;

  printf("Success: %s\n", patch->Success ? "true" : "false");

  printf("PatchData.ID = %u\n", patch->PatchData.ID);
  printf("PatchData.Title = %s\n", patch->PatchData.Title.chars);
  printf("PatchData.ConsoleID = %u\n", patch->PatchData.ConsoleID);
  printf("PatchData.ForumTopicID = %u\n", patch->PatchData.ForumTopicID);
  printf("PatchData.Flags = %u\n", patch->PatchData.Flags);
  printf("PatchData.ImageIcon = %s\n", patch->PatchData.ImageIcon.chars);
  printf("PatchData.ImageTitle = %s\n", patch->PatchData.ImageTitle.chars);
  printf("PatchData.ImageIngame = %s\n", patch->PatchData.ImageIngame.chars);
  printf("PatchData.ImageBoxArt = %s\n", patch->PatchData.ImageBoxArt.chars);
  printf("PatchData.Publisher = %s\n", patch->PatchData.Publisher.chars);
  printf("PatchData.Developer = %s\n", patch->PatchData.Developer.chars);
  printf("PatchData.Genre = %s\n", patch->PatchData.Genre.chars);
  printf("PatchData.Released = %s\n", patch->PatchData.Released.chars);
  printf("PatchData.IsFinal = %s\n", patch->PatchData.IsFinal ? "true" : "false");
  printf("PatchData.ConsoleName = %s\n", patch->PatchData.ConsoleName.chars);
  printf("PatchData.RichPresencePatch = %s\n", patch->PatchData.RichPresencePatch ? patch->PatchData.RichPresencePatch->chars : "(null)");

  for (unsigned j = 0; j < patch->PatchData.Achievements.count; j++)
  {
    Achievement* a = (Achievement*)DEJSON_GET_ELEMENT(patch->PatchData.Achievements, j);

    printf("PatchData.Achievements[%u].ID = %u\n", j, a->ID);
    printf("PatchData.Achievements[%u].MemAddr = %s\n", j, a->MemAddr.chars);
    printf("PatchData.Achievements[%u].Title = %s\n", j, a->Title.chars);
    printf("PatchData.Achievements[%u].Description = %s\n", j, a->Description.chars);
    printf("PatchData.Achievements[%u].Points = %u\n", j, a->Points);
    printf("PatchData.Achievements[%u].Author = %s\n", j, a->Author.chars);
    printf("PatchData.Achievements[%u].Modified = %llu\n", j, a->Modified);
    printf("PatchData.Achievements[%u].Created = %llu\n", j, a->Created);
    printf("PatchData.Achievements[%u].BadgeName = %s\n", j, a->BadgeName.chars);
    printf("PatchData.Achievements[%u].Flags = %u\n", j, a->Flags);
  }

  for (unsigned j = 0; j < patch->PatchData.Leaderboards.count; j++)
  {
    Leaderboard* a = (Leaderboard*)DEJSON_GET_ELEMENT(patch->PatchData.Leaderboards, j);

    printf("PatchData.Leaderboards[%u].ID = %u\n", j, a->ID);
    printf("PatchData.Leaderboards[%u].Mem = %s\n", j, a->Mem.chars);
    printf("PatchData.Leaderboards[%u].Format = %s\n", j, a->Format.chars);
    printf("PatchData.Leaderboards[%u].Title = %s\n", j, a->Title.chars);
    printf("PatchData.Leaderboards[%u].Description = %s\n", j, a->Description.chars);
  }

  //dejson_destroy(&settings);

  return 0;
}
